/**
 * @file commands.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2022-11-08
 * 
 * what's in the command buffer is KISS encoded, but guaranteed by design to be ASCII printable characters, so there's no need to decode it.
 * a command packet has multiple parts (but we don't really care about a lot of it), so I've opted not to create a packet structure or class beyond
 * grabbing the command byte.  If we ever decide to look at the CRC bytes that might change.
 * KISS delimiter, KISS command byte, maybe some data, KISS delimiter
 * length in most cases will be 1 (should test that too) 
 *
 * generically speaking, processcmdbuff is only executed on data from Serial0 (ground or Avionics).  We expect that the TNC interface will be TNC0, so that the
 * port address used by TNCattach will be 0 as well.  Is there any way for the RPi to ever assign the interface a different port address?
 *
 * I'm using KISS command 0xAA to identify data from Serial0 that's destined for Serial0 on the other end of the link.  0xA? is above the port enumeration range of TNCattach.
 
 * for commands or responses bound for the other side, I'm adding a new command code back on.
 * I am using an 0xAA to indicate it's for Serial0 (Avionics)...normal data from Payload has 0x00 for the port address, which
 * should be 0 since it's port AX0 (from TNCattach).  I'm using 0xAA to be even safe since that's not a valid port (only valid ports are 0-9).
 * to be careful (and more generic, only the address nibble is 0 for AX0, if we happen to enumerate to AX1, then the address might be 0x10)  Can this happen?
 * the command byte is unconditionally changed to 0xAA.  This works because processcmdbuff is ONLY run for packets from Serial0.  
 * We know that Serial0 is used are for local or remote commands only.
 * 
 * Commands are structured to be simple (except perhaps for the testing support ones, which can do scans).  They have a generic format: ack, act, respond.
 *
 * it would be nice to synchronize this to the packet boundary of the databuffer.  However, there's no guarantee that there will be a complete packet in the databuffer
 * when a complete command is received (they happen asynchronously)
 * we could wait for it to finish (by looking at the top of the databuffer and seeing if it's 0xC0, if not, we're not at a packet boundary)
 * and then wait for it (most cycles around the loop are completed in the time to receive one byte via serial)
 * or we could work backwards and try to insert it into the buffer (which would involve taking bytes off the stack until we get to a packet boundary, writing the packet 
 * and then putting the bytes back...yeeech)
 * my original thought was that this was only needed for a halt, so it wasn't a big deal if we just trashed the last data packet
 * HOWEVER, there's the question of doppler correction, which would be done by ground sending periodic frequency change commands.
 * they have to be processed in the context of a continuing data transfer, and it would be bad to trash packets.  So we gotta sync.
 * I *think* the way to handle it is to ONLY process commands if the top of the databuff is 0xC0 or empty.  (check both).  It would effectively delay command processing,
 * but only if data is coming in at the same time.  (net effect..none here, it gets taken care of in the main code, and HAS been implemented)
 * 
 * note: commands have variable amounts of data, but these are generally some fixed amount per command. More generically, the data we
 * receive after the first C0 and command code, and the last C0 is the length of the data in the command.  
 * So, it might be better to move that into a helper function that extracts the data. (but i'm leaving it for now)
 * each command can then individually check the length and process the data as needed.  (basically the beginnings of the command packet class...)
 */

//#include "ax.h"
#include "commands.h"
#include "constants.h"

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

void processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE>& cmdbuffer, CircularBuffer<byte, DATABUFFSIZE>& databuffer, int packetlength, ax_config& config, ax_modulation& modulation, bool& transmit, int& offset)
{
  // first remove the seal... 0xC0
  cmdbuffer.shift();
  // and then grab the command code  
  byte commandcode = cmdbuffer.shift();  
  debug_printf("command code is: %x \r\n", commandcode);

  // now let's process the command code
  switch (commandcode) {
    case 0xAA:
    case 0:
      // nothing to see here, it's not for me...forward to the other end, so copy this over to the tx buffer
      {
        databuffer.push(constants::FEND);
        // so for commands or responses bound for the other side, I'm adding a new command code back on to indicate where it's going.
        databuffer.push(0xAA);
        // you're starting at the second byte of the total packet
        for (int i = 2; i < packetlength; i++) {
          // shift it out of cmdbuffer and push it into databuffer, don't need to push a final 0xC0 because it's still part of the packet
          databuffer.push(cmdbuffer.shift());        
        }      
      break;
      }

    case 0x07:  // Beacon
      {
        // acknowledge command
          sendACK(commandcode);
        
        // act on command
          // beaconstring consists of callsign (6 bytes), a space, and four beacon characters (4 bytes) + plus terminator (1 byte)
          byte beacondata[12] {};
          memcpy(beacondata, constants::callsign, sizeof(constants::callsign));
          debug_printf("size of callsign %x \r\n", sizeof(constants::callsign));
          beacondata[6] = 0x20; // add a space

          //copy in the beacon data from the cmdbuffer
          for (int i = 7; i < 10; i++) { // avionics is now only sending 3 status bytes (avionics, payload, eps)
            beacondata[i] = cmdbuffer.shift();  // pull out the data bytes in the buffer (command data or response)
          }

          beacondata[10] = 0x45; // placeholder for radio status byte
          beacondata[11] = 0;  // add null terminator
          int beaconstringlength = sizeof(beacondata);
          debug_printf("beacondata = %12c \r\n", beacondata);
        
          sendbeacon(beacondata, beaconstringlength, config, modulation);
          
        // respond to command
          // beacon has no response

        cmdbuffer.shift();  //remove the last C0
        break;
      }

    case 0x08:  // Manual Antenna Release
      {
        // acknowledge command
          sendACK(commandcode);

        // act on command
          char select = cmdbuffer.shift();
          String response{};

          if (select == 0x43)
          {
            digitalWrite(Release_A, 1);
            digitalWrite(Release_B, 1);
            delay(30000);
            digitalWrite(Release_A, 0);
            digitalWrite(Release_B, 0);
            response = "Both cycles complete";
          }

          else if (select == 0x42)
          {
            digitalWrite(Release_A, 0);
            digitalWrite(Release_B, 1);
            delay(30000);
            digitalWrite(Release_A, 0);
            digitalWrite(Release_B, 0);
            response = "Release_B cycle complete";
          }

          else if (select == 0x41)
          {
            digitalWrite(Release_A, 1);
            digitalWrite(Release_B, 0);
            delay(30000);
            digitalWrite(Release_A, 0);
            digitalWrite(Release_B, 0);
            response = "Release_A cycle complete";
          }

        // respond to command
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x09:  // Radio Status
      {
        // acknowledge command
          sendACK(commandcode);
        
        // act on command
          String response{};
          int reportlength = reportstatus(response, config, modulation);  // the status should just be written to a string somewhere, or something like that.
          Serial.println(response);

        // respond to command
          if (reportlength < 200)
          {
            sendResponse(commandcode, response);
          }
          else
          {
            response = "status string too long...go fix it";
            sendResponse(commandcode, response);
          }

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x0A:  // Halt - clear buffers, halt radio transmissions, revert to RX state and report if successful
      {
        // acknowledge command
          sendACK(commandcode);

        // act on command
          debug_printf("clearing the data buffer \r\n");
          databuffer.clear();
          
          debug_printf("clearing the AX5043 FIFO");  // may be unnecessary...may have unintended consequences?
          ax_fifo_clear(&config);

          // assuming for now that I don't need to clear the transmit buffer.  Need to verify this.
          debug_printf("resetting radio to receive state \r\n");
          ax_init(&config);  // this does a reset, so needs to be first
          ax_default_params(&config, &modulation);  // load the current RF modulation parameters for the current config
          ax_rx_on(&config, &modulation);
          transmit = false;

        // respond to command
          // halt command has no response

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x0B:  // Modify Frequency.  Frequency is part of the ax_config structure
      {
        // acknowledge command
          sendACK(commandcode);
      
        // act on command
          char freqstring[10];
          for (int i = 0; i < 9; i++) {
            freqstring[i] = (char)cmdbuffer.shift();  // pull out the data bytes in the buffer (command data or response)
          }
          freqstring[9] = 0;
          // convert string to integer, modify config structure and implement change on radio
          ax_adjust_frequency(&config, atoi(freqstring));
          debug_printf("new frequency: %s \r\n", freqstring);
          config.synthesiser.A.frequency = atoi(freqstring);
          config.synthesiser.B.frequency = atoi(freqstring);

        // respond to command
          String response(freqstring);
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x0C:  // Modify Mode - change the operating mode by index. 0 = FSK, 1 = GMSK, 2 = GMSK with FEC
        {
          // acknowledge command
            sendACK(commandcode);

          // act on command
            char mode_index = cmdbuffer.shift();
            if (mode_index == 0x00)
            {
              modulation.fec = 0;
              modulation.shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED;
              modulation.bitrate = 9600;
              ax_init(&config);  // this does a reset, so needs to be first
              // load the RF parameters for the current config
              ax_default_params(&config, &modulation);  //ax_modes.c for RF parameters
              ax_rx_on(&config, &modulation);
            }
            else if (mode_index == 0x01)
            {
              modulation.fec = 0;
              modulation.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
              modulation.bitrate = 9600;
              ax_init(&config);  //this does a reset, so needs to be first
              // load the RF parameters for the current config
              ax_default_params(&config, &modulation);  //ax_modes.c for RF parameters
              ax_rx_on(&config, &modulation);
            }
            else if (mode_index == 0x02)
            {
              //ax_MODIFY_FEC(&config, &modulation, true);
              //ax_MODIFY_SHAPING(&config, &modulation, 1);
              modulation.fec = 1;
              modulation.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
              modulation.bitrate = 19200;
              ax_init(&config);  //this does a reset, so needs to be first
              // load the RF parameters for the current config
              ax_default_params(&config, &modulation);  //ax_modes.c for RF parameters
              ax_rx_on(&config, &modulation);
            }
            else
            {
              debug_printf("ERROR: index out of bounds");
            }

          // send response
          cmdbuffer.shift();  // remove the last C0
          break;
        }

    case 0x0D:  // Doppler Offset
      {
        // acknowledge command
          sendACK(commandcode);

        // act on command
          // this grabs the value from the command and updates the 
          // the offset needs to be applied in the main program
          char sign = cmdbuffer.shift();
          char offset_string[6];
          for (int i = 1; i < 6; i++)
          {
            offset_string[i] = (char)cmdbuffer.shift();
            offset_string[6] = 0;
            offset = atoi(offset_string);  //offset is a signed integer
            if (sign == 0x01)
            {
              offset = -offset;
            }
            debug_printf("offset value is: %i", offset);
          }

        // send response
          String response = "Offset set to " + String(offset, DEC);
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }  

    case 0x0E:  // send Call sign command  - this just sends a packet with the callsign as the data
      {
        // acknowledge Call Sign command
          sendACK(commandcode);

        // act on command
          databuffer.push(constants::FEND);
          databuffer.push(0xAA);
          for (int i = 0; i < sizeof(constants::callsign)-1; i++) {  
            databuffer.push(constants::callsign[i]);        
          }
          databuffer.push(constants::FEND);

        // call sign command has no response

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x17:  // CW command - this is "mostly CW".  In close, it looks like the carrier is still modulated.  It's the same on the stock dev boards, so i don't think it's me.
      {
        // acknowledge command
          sendACK(commandcode);

        // act on command
          char durationstring[3];
          durationstring[0] = (char)cmdbuffer.shift();
          durationstring[1] = (char)cmdbuffer.shift();
          durationstring[2] = 0;
          int duration = atoi(durationstring);
          debug_printf("duration: %u \r\n", duration);

          if (duration <= 1)
          {
            duration = 1;
          }
          else if (duration >= 99)
          {
            duration = 99;
          }
          
          ax_init(&config);  // do an init first
          // modify the power to match what's in the modulation structure...make sure the modulation type matches
          // this keeps beacon at full power
          
          //debug_printf("ask power: %d \r\n", ask_modulation.power); //check to make sure it was modified...but maybe it wasn't?
          
          ax_default_params(&config, &ask_modulation);  // load the RF parameters
            
          pinfunc_t func = 0x84;               // set for wire mode
          ax_set_pinfunc_data(&config, func);  // remember to set this back when done!

          // set the RF switch to transmit
          digitalWrite(TX_RX, HIGH);
          digitalWrite(RX_TX, LOW);
          digitalWrite(AX5043_DATA, HIGH);

          ax_tx_on(&config, &ask_modulation);  // turn on the transmitter

          // start transmitting
          debug_printf("output CW for %u seconds \r\n", duration);
          digitalWrite(PAENABLE, HIGH);
          // delay(PAdelay); //let the pa bias stabilize
          digitalWrite(PIN_LED_TX, HIGH);
          digitalWrite(AX5043_DATA, HIGH);

          delay(duration * 1000);

          // stop transmitting
          digitalWrite(AX5043_DATA, LOW);
          digitalWrite(PAENABLE, LOW);  //turn off the PA
          digitalWrite(PIN_LED_TX, LOW);
          debug_printf("done \r\n");

          // drop out of wire mode
          func = 2;
          ax_set_pinfunc_data(&config, func);

          // now put it back the way you found it.
          ax_init(&config); // do a reset
          ax_default_params(&config, &modulation);  // ax_modes.c for RF parameters
          debug_printf("default params loaded \r\n");
          //Serial.println("default params loaded \r\n");
          ax_rx_on(&config, &modulation);
          debug_printf("receiver on \r\n");
          //Serial.println("receiver on \r\n");

        // send response
          String response = "CW Mode complete";
          sendResponse(commandcode, response); 

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x18:  //Background RSSI
      {
        // acknowledge command
          sendACK(commandcode);  //ack the command and get the parameters

        // act on command
          // dwell time per step
          char integrationtime_string[3];  //to hold the number of steps (3 bytes + null)
          for (int i = 0; i < 2; i++) {
            integrationtime_string[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
          }
          integrationtime_string[2] = 0;  //set the terminator
          debug_printf("integration time: %s \r\n", integrationtime_string);

          unsigned long integrationtime = (unsigned long)atoi(integrationtime_string);
          unsigned long starttime = millis();
          int rssi_sum {0};
          // byte rssi;
          unsigned long count {0};

          do {
            rssi_sum += ax_RSSI(&config);   // just keep summing up readings as an unsigned 32 bit value
            count++;  // and keep a count of how many times you did it
            delay(100); // let's arbitrarily delay 100mS between readings, so about number of readings is about 10x the integration time 
          }while ((millis() - starttime) < integrationtime*1000);
          
          int background_rssi = rssi_sum/count;
          debug_printf("background rssi: %u \r\n", background_rssi);
          debug_printf("rssi sum: %u \r\n", rssi_sum);
          debug_printf("count: %lu \r\n", count);

        // send response
          String background_rssi_str(background_rssi, DEC);
          sendResponse(commandcode, background_rssi_str);

        cmdbuffer.shift();  // remove the last C0
        break;  
      }

    case 0x19:  //Current RSSI - quick and dirty RSSI measurement
      {
        // acknowledge command
          sendACK(commandcode);  //ack the command and get the parameters

        // act on command
          byte rssi = ax_RSSI(&config);
          String rssi_str(rssi, DEC);

        // send response
          sendResponse(commandcode, rssi_str);

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x1A: //Sweep Transmitter
      {        
        // acknowledge command
          sendACK(commandcode);  //ack the command

        // act on command
          // get the parameters
          // frequency is part of the ax_config structure
          // start frequency
          char startfreqstring[10];  // to hold the beacon data (9 bytes + null)
          for (int i = 0; i < 9; i++) {
            startfreqstring[i] = (char)cmdbuffer.shift();  // pull out the data bytes in the buffer (command data or response)
          }
          startfreqstring[9] = 0;  // set the terminator
          debug_printf("start frequency: %s \r\n", startfreqstring);
          //stop frequency
          char stopfreqstring[10];  // to hold the beacon data (9 bytes + null)
          for (int i = 0; i < 9; i++) {
            stopfreqstring[i] = (char)cmdbuffer.shift();  // pull out the data bytes in the buffer (command data or response)
          }
          stopfreqstring[9] = 0;  // set the terminator
          debug_printf("stop frequency: %s \r\n", stopfreqstring);
          // number of steps
          char numberofstepsstring[4];  // to hold the number of steps (3 bytes + null)
          for (int i = 0; i < 3; i++) {
            numberofstepsstring[i] = (char)cmdbuffer.shift();  // pull out the data bytes in the buffer (command data or response)
          }
          numberofstepsstring[3] = 0;  // set the terminator
          debug_printf("number of steps: %s \r\n", numberofstepsstring);
          // dwell time per step
          char dwellstring[4];  // to hold the number of steps (3 bytes + null)
          for (int i = 0; i < 3; i++) {
            dwellstring[i] = (char)cmdbuffer.shift();  // pull out the data bytes in the buffer (command data or response)
          }
          dwellstring[3] = 0;  //set the terminator
          debug_printf("dwell time: %s \r\n", dwellstring);

          int startfreq = atoi(startfreqstring);  
          int stopfreq = atoi(stopfreqstring);
          int numsteps = atoi(numberofstepsstring);
          int dwelltime = atoi(dwellstring);
          int stepsize = (int)((stopfreq - startfreq) / numsteps);  // find the closest integer to the step size
          debug_printf("stepsize = %u \r\n", stepsize);

          config.synthesiser.A.frequency = startfreq;
          config.synthesiser.B.frequency = startfreq;

          ax_init(&config);                             // do an init first
          ax_default_params(&config, &ask_modulation);  // load the RF parameters
          digitalWrite(AX5043_DATA, LOW);

          pinfunc_t func = 0x84;               // set for wire mode
          ax_set_pinfunc_data(&config, func);  // remember to set this back when done!

          // set the RF switch to transmit
          digitalWrite(TX_RX, HIGH);
          digitalWrite(RX_TX, LOW);
          /*
          //this is the fast method
          ax_tx_on(&config, &ask_modulation);  //turn on the transmitter

          for (int j = startfreq; j <= stopfreq; j += stepsize) {
            debug_printf("current frequency: %u \r\n", j);
            ax_force_quick_adjust_frequency(&config, j);

            //start transmitting
            debug_printf("output for %u milliseconds \r\n", dwelltime);
            digitalWrite(PAENABLE, HIGH);
            //delay(PAdelay); //let the pa bias stabilize
            digitalWrite(PIN_LED_TX, HIGH);
            digitalWrite(AX5043_DATA, HIGH);

            delay(dwelltime);

            //stop transmitting
            digitalWrite(AX5043_DATA, LOW);
            digitalWrite(PAENABLE, LOW);  //turn off the PA
            digitalWrite(PIN_LED_TX, LOW);
            debug_printf("done \r\n");
          }
          */

          // this is the slow method
          ax_rx_on(&config, &ask_modulation);  // start with in full_rx state
          for (int j = startfreq; j <= stopfreq; j += stepsize) {
            debug_printf("current frequency: %d \r\n", j);
            ax_adjust_frequency(&config, j);
            ax_tx_on(&config, &ask_modulation); 
            //start transmitting
            debug_printf("output for %d milliseconds \r\n", dwelltime);
            digitalWrite(PAENABLE, HIGH);
            //delay(PAdelay); // let the pa bias stabilize
            digitalWrite(PIN_LED_TX, HIGH);
            digitalWrite(AX5043_DATA, HIGH);

            delay(dwelltime);

            // stop transmitting
            digitalWrite(AX5043_DATA, LOW);
            digitalWrite(PAENABLE, LOW);  // turn off the PA
            digitalWrite(PIN_LED_TX, LOW);
            debug_printf("done \r\n");
            ax_set_pwrmode(&config, AX_PWRMODE_STANDBY);  // go into standby..should preserve registers
            while (ax_RADIOSTATE(&config) == AX_RADIOSTATE_TX); // idle here until it clears
          }

          // drop out of wire mode
          func = 2;
          ax_set_pinfunc_data(&config, func);

          // now put it back the way you found it.
          ax_init(&config);
          ax_default_params(&config, &modulation);
          debug_printf("default params loaded \r\n");
          ax_rx_on(&config, &modulation);
          debug_printf("receiver on \r\n");

        // send response
          String response = "sweep complete, parked at last frequency";
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x1B:  // Sweep Receiver
      {
        // ack command
          sendACK(commandcode);  //ack the command and get the parameters

        // act on command
          //PLACEHOLDER

        // send response
          String response = "not yet implemented";
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x1C:  // Query Radio Register
      {
        // ack command
          sendACK(commandcode);  //ack the command and get the parameters

        // act on command
          char ax5043_register[6];
          for (int i=0; i<5; i++)
          {
            ax5043_register[i] = cmdbuffer.shift();
          }
          ax5043_register[5] = 0;
          int ax5043_register_int = atoi(ax5043_register);

          String response = "Register Value: ";
          response += String(ax_hw_read_register_8(&config, ax5043_register_int));

        // send response
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }
    
    case 0x1D:  // Adjust Output Power
      {
        // ack command
          sendACK(commandcode);  //ack the command and get the parameters

        // act on command
          char power_str[4];
          for (int i=0; i<3; i++)
          {
            power_str[i] = cmdbuffer.shift();
          }
          int power = atoi(power_str);
          if (power < 10)
          {
            power = 10;
          }
          else if (power > 100)
          {
            power = 100;
          }
          
          // ax_MODIFY_TX_POWER(&config, power/100);  //loads a new power level, but doesn't modify the structure
          float power_frac  = float(power)/100.0;
          modulation.power = power_frac;
          debug_printf("new power level is: %d \r\n", power);
          debug_printf("new power fraction is: %f \r\n", power_frac);
          //now reload the configuration into the AX5043
          ax_init(&config);                             //do a reset
          ax_default_params(&config, &modulation);  //ax_modes.c for RF parameters
          debug_printf("default params loaded \r\n");
          debug_printf("power level: %f \r\n", modulation.power);
          //Serial.println("default params loaded \r\n");
          ax_rx_on(&config, &modulation);
          debug_printf("receiver on \r\n");
          
        // send response
          String response = ("New power level is: " + String(power_frac, 3));
          sendResponse(commandcode, response);

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    default:
      sendNACK(commandcode);
      cmdbuffer.shift();  //remove the last C0
      break;
  }
}


void sendACK(byte code) 
{
  //create an ACK packet and send it out Serial0 - for testing at this moment just sent it to Serial
  //note that acks always go to Serial0
  debug_printf("ACK!! \r\n");

  byte ackpacket[] = { 0xC0, 0x00, 0x41, 0x43, 0x4B, 0x00, 0xC0 };  //generic form of ack packet
  ackpacket[5] = code;                                                       //replace code byte with the received command code

  Serial0.write(ackpacket, 7);
}


void sendNACK(byte code) 
{
  //create an NACK packet and put it in the CMD TX queue
  //nacks always go to Serial0
  debug_printf("NACK!! \r\n");                                                        //bad code, no cookie!
  byte nackpacket[] = { 0xC0, 0x00, 0x4E, 0x41, 0x43, 0x4B, 0x00, 0xC0 };  //generic form of nack packet
  nackpacket[6] = code;                                                             //replace code byte with the received command code

  Serial0.write(nackpacket, 8);
}


//send a response.  currently assumed that response is a character string: this function is responsible for converting response to bytes and sending it out as KISS packet
void sendResponse(byte code, String& response) 
{
  debug_printf("Sending the response \r\n");

  //responses are KISS with cmd byte = 0x00, and always start with 'RES'
  byte responsestart[6]{ 0xC0, 0x00, 0x52, 0x45, 0x53, 0x00 };
  responsestart[5] = code;
  byte responseend[1]{ 0xC0 };  //placeholder for more if we need it

  byte responsebuff[200];                          //create a buffer for the response bytes
  response.getBytes(responsebuff, response.length() + 1);  //get the bytes

  //write it to Serial0 in parts
  Serial0.write('\n');
  Serial0.write(responsestart, 6);                 // first header
  Serial0.write(responsebuff, response.length());  //now the actual data
  Serial0.write(responseend, 1);                   //and finish the KISS packet
}


size_t reportstatus(String& response, ax_config& config, ax_modulation& modulation)
{
  // create temperature sensor instance, only needed here
  Generic_LM75_10Bit tempsense(0x4B);

  response = "Freq:" + String(config.synthesiser.A.frequency, DEC);
  response += "; Status:" + String(ax_hw_status(), HEX);
  float patemp { tempsense.readTemperatureC() };
  response += "; Temp: " + String(patemp, 1);
  uint8_t overcurrent = digitalRead(OC5V);
  Serial0.println(overcurrent, HEX);
  response += "; OC5V: " + String(overcurrent, HEX);
  response += "; Shape:" + String(modulation.shaping, HEX);
  response += "; FEC:" + String(modulation.fec, HEX);
  response += "; Bitrate:" + String(modulation.bitrate, DEC);
  response += "; Pwr%:" + String(modulation.power, 3);
  
  // response = "generic response";
  return response.length();
}