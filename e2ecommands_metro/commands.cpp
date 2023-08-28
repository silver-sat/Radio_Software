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
 * Commands are structured to be simple (except perhaps for the testing support ones, which can do scans)
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
 */

//#include "ax.h"
#include "commands.h"
#include "constants.h"

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

void processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE>& cmdbuffer, CircularBuffer<byte, DATABUFFSIZE>& databuffer, int packetlength, ax_config& config) {
  
  cmdbuffer.shift();                              //remove the seal... C0
  byte commandcode = cmdbuffer.shift();  //and the command code  
  debug_printf("command code is: %x \n", commandcode);

  /*
  char commandstring[packetlength - 2] {}; //extract the rest of the buffer, length is less 2 C0's and command byte plus null terminator
  for (int i = 0; i< packetlength - 3; i++){
    commandstring[i] = cmdbuffer.shift(); //shift out the entire command except for the 2 C0's and command byte.
    debug_printf("commandstring byte %x = %x \n", i, commandstring[i]);
  }
  cmdbuffer.shift(); //shift out last C0
  commandstring[packetlength - 3] = '\0';  //put null term on the commandstring 
  debug_printf("commandstring = %s \n", commandstring);
  Serial.println(commandstring);
  */

  switch (commandcode) {
    case 0xAA:  
    case 0:     //nothing to see here, it's not for me...forward to the other end, so copy this over to the tx buffer
      {
        databuffer.push(FEND);
        //so for commands or responses bound for the other side, I'm adding a new command code back on.  However, why not just make this the address byte?
        //I am using an 0xAA to indicate it's for Serial0 (Avionics)...normal data from Payload will likely have 0x00, where ? is the Port index nibble, which
        //should be 0 since it's port AX0.  I'm using 0xAA to be even safe since that's not a valid port (only valid ports are 0-9).
        databuffer.push(0xAA);
        for (int i = 2; i < packetlength; i++) {  //you're starting at the second byte of the total packet
          databuffer.push(cmdbuffer.shift());        //shift it out of cmdbuffer and push it into databuffer, don't need to push a final 0xC0 because it's still part of the packet
        }      
      break;
      }

    case 0x07:
      {
        //acknowledge beacon
        sendACK(commandcode);
        
        //action beacon
        byte beacondata[12] {}; //beaconstring consists of callsign (6 bytes), a space, and four beacon characters (4 bytes) + plus terminator (1 byte)
    
        memcpy(beacondata, constants::callsign, sizeof(constants::callsign)); //copy in the callsign
        debug_printf("size of callsign %x \n", sizeof(constants::callsign));

        beacondata[6] = 0x20; //add a space

        //copy in the beacon data from the cmdbuffer
        for (int i = 7; i < 10; i++) { //avionics is now only sending 3 status bytes (avionics, payload, eps)
          beacondata[i] = cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }

        beacondata[10] = 0x65; //placeholder for radio status byte
        beacondata[11] = 0;  //add null terminator

        int beaconstringlength = sizeof(beacondata);

        //debug_printf("size of beacondata %x \n", sizeof(beacondata));
        debug_printf("beacondata = %12c \n", beacondata);
        
        cmdbuffer.shift();  //remove the last C0
        sendbeacon(beacondata, beaconstringlength, config);

        //beacon has no response

        break;
      }

    case 0x08:
      {
        //deploy the antenna and report if successful
        //acknowledge deploy antenna command
        sendACK(commandcode);

        //act on command
        String response{};
        deployantenna(response);  //how long should this take?

        //respond to command
        sendResponse(commandcode, response);
        cmdbuffer.shift();  //remove the last C0
        break;
      }

    case 0x09:
      {
        //status
        sendACK(commandcode);
        String response{};
        reportstatus(response, config);  //the status should just be written to a string somewhere, or something like that.
        sendResponse(commandcode, response);
        cmdbuffer.shift();  //remove the last C0
        break;
      }

    case 0x0A:
      {
        //halt radio transmissions, revert to RX state and report if successful
        sendACK(commandcode);
        haltradio();
        cmdbuffer.shift();  //remove the last C0
        break;
      }

    case 0x0C:
      {
        //change operating mode...not implemented yet
        sendACK(commandcode);
        cmdbuffer.shift();  //remove the last C0
        break;
      }

    case 0x0B:
    case 0x0D:
      {
        //adjust frequency
        //frequency is part of the ax_config structure

        char freqstring[10];  //to hold the beacon data (9 bytes + null)
        for (int i = 0; i < 9; i++) {
          freqstring[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }
        freqstring[9] = 0;
        debug_printf("new frequency: %s \n", freqstring);
        sendACK(commandcode);
        ax_adjust_frequency(&config, atoi(freqstring));

        cmdbuffer.shift();  // remove the last C0

        String response(freqstring);
        sendResponse(commandcode, response);
        break;
      }

    case 0x17:
      //ack CW command
      {
        char durationstring[3];
        durationstring[0] = (char)cmdbuffer.shift();
        durationstring[1] = (char)cmdbuffer.shift();
        durationstring[2] = 0;
        int duration = atoi(durationstring);
        debug_printf("duration: %u \n", duration);
        if (duration >= 1 && duration <= 99) {
          sendACK(commandcode);

          ax_init(&config);                             //do an init first
          ax_default_params(&config, &ask_modulation);  //load the RF parameters

          pinfunc_t func = 0x84;               //set for wire mode
          ax_set_pinfunc_data(&config, func);  //remember to set this back when done!

          //set the RF switch to transmit
          digitalWrite(TX_RX, HIGH);
          digitalWrite(RX_TX, LOW);

          ax_tx_on(&config, &ask_modulation);  //turn on the transmitter

          //start transmitting
          debug_printf("output CW for %u seconds \n", duration);
          digitalWrite(PAENABLE, HIGH);
          //delay(PAdelay); //let the pa bias stabilize
          digitalWrite(PIN_LED_TX, HIGH);
          digitalWrite(AX5043_DATA, HIGH);

          delay(duration * 1000);

          //stop transmitting
          digitalWrite(AX5043_DATA, LOW);
          digitalWrite(PAENABLE, LOW);  //turn off the PA
          digitalWrite(PIN_LED_TX, LOW);
          debug_printf("done \n");

          //drop out of wire mode
          func = 1;
          ax_set_pinfunc_data(&config, func);

          //now put it back the way you found it.
          ax_init(&config);                             //do a reset
          ax_default_params(&config, &fsk_modulation);  //ax_modes.c for RF parameters
          debug_printf("default params loaded \n");
          //Serial.println("default params loaded \n");
          ax_rx_on(&config, &fsk_modulation);
          debug_printf("receiver on \n");
          //Serial.println("receiver on \n");

          String response = "CW Mode complete";
          sendResponse(commandcode, response);
        } 
        else {
          sendNACK(commandcode);
        }
        cmdbuffer.shift();  //remove the last C0
        break;
      }

    case 0x18:
      {
        //Background RSSI
        sendACK(commandcode);  //ack the command and get the parameters
        //dwell time per step
        char integrationtime_string[3];  //to hold the number of steps (3 bytes + null)
        for (int i = 0; i < 2; i++) {
          integrationtime_string[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }
        integrationtime_string[2] = 0;  //set the terminator
        debug_printf("integration time: %s \n", integrationtime_string);

        unsigned long integrationtime = (unsigned long)atoi(integrationtime_string);
        unsigned long starttime = millis();
        int rssi_sum {0};
        // byte rssi;
        unsigned long count {0};

        do {
          rssi_sum += ax_RSSI(&config);   //just keep summing up readings as an unsigned 32 bit value
          count++;  //and keep a count of how many times you did it
          delay(100); //let's arbitrarily delay 100mS between readings, so about number of readings is about 10x the integration time 
        }while ((millis() - starttime) < integrationtime*1000);
        
        int background_rssi = rssi_sum/count;
        debug_printf("background rssi: %u \n", background_rssi);
        debug_printf("rssi sum: %u \n", rssi_sum);
        debug_printf("count: %lu \n", count);

        String background_rssi_str(background_rssi, DEC);
        sendResponse(commandcode, background_rssi_str);
        break;  
      }

    case 0x19:
      {
        //Current RSSI
        sendACK(commandcode);  //ack the command and get the parameters
        byte rssi = ax_RSSI(&config);
        String rssi_str(rssi, DEC);
        sendResponse(commandcode, rssi_str);
        break;
      }

    case 0x1A:
      {
        //Sweep Transmitter
        //frequency is part of the ax_config structure
        sendACK(commandcode);  //ack the command

        //get the parameters
        //start frequency
        char startfreqstring[10];  //to hold the beacon data (9 bytes + null)
        for (int i = 0; i < 9; i++) {
          startfreqstring[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }
        startfreqstring[9] = 0;  //set the terminator
        debug_printf("start frequency: %s \n", startfreqstring);
        //stop frequency
        char stopfreqstring[10];  //to hold the beacon data (9 bytes + null)
        for (int i = 0; i < 9; i++) {
          stopfreqstring[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }
        stopfreqstring[9] = 0;  //set the terminator
        debug_printf("stop frequency: %s \n", stopfreqstring);
        //number of steps
        char numberofstepsstring[4];  //to hold the number of steps (3 bytes + null)
        for (int i = 0; i < 3; i++) {
          numberofstepsstring[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }
        numberofstepsstring[3] = 0;  //set the terminator
        debug_printf("number of steps: %s \n", numberofstepsstring);
        //dwell time per step
        char dwellstring[4];  //to hold the number of steps (3 bytes + null)
        for (int i = 0; i < 3; i++) {
          dwellstring[i] = (char)cmdbuffer.shift();  //pull out the data bytes in the buffer (command data or response)
        }
        dwellstring[3] = 0;  //set the terminator
        debug_printf("dwell time: %s \n", dwellstring);

        int startfreq = atoi(startfreqstring);  
        int stopfreq = atoi(stopfreqstring);
        int numsteps = atoi(numberofstepsstring);
        int dwelltime = atoi(dwellstring);
        int stepsize = (int)((stopfreq - startfreq) / numsteps);  //find the closest integer to the step size
        debug_printf("stepsize = %u \n", stepsize);

        config.synthesiser.A.frequency = startfreq;
        config.synthesiser.B.frequency = startfreq;

        ax_init(&config);                             //do an init first
        ax_default_params(&config, &ask_modulation);  //load the RF parameters
        digitalWrite(AX5043_DATA, LOW);

        pinfunc_t func = 0x84;               //set for wire mode
        ax_set_pinfunc_data(&config, func);  //remember to set this back when done!

        //set the RF switch to transmit
        digitalWrite(TX_RX, HIGH);
        digitalWrite(RX_TX, LOW);
        /*
        //this is the fast method
        ax_tx_on(&config, &ask_modulation);  //turn on the transmitter

        for (int j = startfreq; j <= stopfreq; j += stepsize) {
          debug_printf("current frequency: %u \n", j);
          ax_force_quick_adjust_frequency(&config, j);

          //start transmitting
          debug_printf("output for %u milliseconds \n", dwelltime);
          digitalWrite(PAENABLE, HIGH);
          //delay(PAdelay); //let the pa bias stabilize
          digitalWrite(PIN_LED_TX, HIGH);
          digitalWrite(AX5043_DATA, HIGH);

          delay(dwelltime);

          //stop transmitting
          digitalWrite(AX5043_DATA, LOW);
          digitalWrite(PAENABLE, LOW);  //turn off the PA
          digitalWrite(PIN_LED_TX, LOW);
          debug_printf("done \n");
        }
        */
        //this is the slow method
        ax_rx_on(&config, &fsk_modulation);  //start with in full_rx state
        for (int j = startfreq; j <= stopfreq; j += stepsize) {
          debug_printf("current frequency: %d \n", j);
          ax_adjust_frequency(&config, j);
          ax_tx_on(&config, &ask_modulation); 
          //start transmitting
          debug_printf("output for %d milliseconds \n", dwelltime);
          digitalWrite(PAENABLE, HIGH);
          //delay(PAdelay); //let the pa bias stabilize
          digitalWrite(PIN_LED_TX, HIGH);
          digitalWrite(AX5043_DATA, HIGH);

          delay(dwelltime);

          //stop transmitting
          digitalWrite(AX5043_DATA, LOW);
          digitalWrite(PAENABLE, LOW);  //turn off the PA
          digitalWrite(PIN_LED_TX, LOW);
          debug_printf("done \n");
          ax_set_pwrmode(&config, AX_PWRMODE_STANDBY);  //go into standby..should preserve registers
          while (ax_RADIOSTATE(&config) == AX_RADIOSTATE_TX); //idle here until it clears
        }

        //drop out of wire mode
        func = 1;
        ax_set_pinfunc_data(&config, func);

        //now put it back the way you found it.
        ax_init(&config);                             //do a reset
        ax_default_params(&config, &fsk_modulation);  //ax_modes.c for RF parameters
        debug_printf("default params loaded \n");
        //Serial.println("default params loaded \n");
        ax_rx_on(&config, &fsk_modulation);
        debug_printf("receiver on \n");
        //Serial.println("receiver on \n");

        String response = "sweep complete";
        sendResponse(commandcode, response);
        //current just ends parket at last frequency

        cmdbuffer.shift();  // remove the last C0
        break;
      }

    case 0x1B:
      {
        //Sweep Receiver
        sendACK(commandcode);  //ack the command and get the parameters
        //PLACEHOLDER
        break;
      }

    default:
      sendNACK(commandcode);
      cmdbuffer.shift();  //remove the last C0
      break;
  }
}

void   sendACK(byte code) {
  //create an ACK packet and send it out Serial0 - for testing at this moment just sent it to Serial
  //note that acks always go to Serial0
  debug_printf("ACK!! \n");

  byte ackpacket[] = { 0xC0, 0x00, 0x41, 0x43, 0x4B, 0x00, 0xC0 };  //generic form of ack packet
  ackpacket[5] = code;                                                       //replace code byte with the received command code

  Serial0.write(ackpacket, 7);
}

void sendNACK(byte code) {
  //create an NACK packet and put it in the CMD TX queue
  //nacks always go to Serial0
  debug_printf("NACK!! \n");                                                        //bad code, no cookie!
  byte nackpacket[] = { 0xC0, 0x00, 0x4E, 0x41, 0x43, 0x4B, 0x00, 0xC0 };  //generic form of nack packet
  nackpacket[6] = code;                                                             //replace code byte with the received command code

  Serial0.write(nackpacket, 8);
}


//send a response.  currently assumed that response is a character string: this function is responsible for converting response to bytes and sending it out as KISS packet
void sendResponse(byte code, String& response) {
  debug_printf("Sending the response \n");

  //responses are KISS with cmd byte = 0x00, and always start with 'RES'
  byte responsestart[6]{ 0xC0, 0x00, 0x52, 0x45, 0x53, 0x00 };
  responsestart[5] = code;
  byte responseend[1]{ 0xC0 };  //placeholder for more if we need it

  byte responsebuff[25];                          //create a buffer for the response bytes
  response.getBytes(responsebuff, response.length() + 1);  //get the bytes

  //write it to Serial0 in parts
  Serial0.write(responsestart, 6);                 // first header
  Serial0.write(responsebuff, response.length());  //now the actual data
  Serial0.write(responseend, 1);                   //and finish the KISS packet
}


size_t deployantenna(String& response) {
  //the only thing the radio has any control over is the two failsafe deployment lines
  //this command is only local, and it's only useful on the satellite
  debug_printf("deploying the antenna and generating a report \n");
  response = "antenna status report";

  return response.length();
}


size_t reportstatus(String& response, ax_config& config) {
  debug_printf("generating the status report \n");
  debug_printf("config variable values: \n");
  debug_printf("tcxo frequency: %d \n", int(config.f_xtal));
  debug_printf("synthesizer A frequency: %d \n", int(config.synthesiser.A.frequency));
  debug_printf("synthesizer B frequency: %d \n", int(config.synthesiser.B.frequency));
  debug_printf("status: %x \n", ax_hw_status());

  //eFuse 5V status
  //PA temperature
  //FEC?
  //RSSI?
  //radio config? (see debug output)

  response = "status report";

  return response.length();
}


void haltradio() {
  //lot going on here too!
  debug_printf("halting all radio activity \n");
}