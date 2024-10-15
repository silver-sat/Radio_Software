/**
 * @file commands.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2024-07-19
 *
 what's in the command buffer is KISS encoded, but guaranteed by design to be ASCII printable characters, so there's no need to decode it.
 a command packet has multiple parts (but we don't really care about a lot of it)
 
 generically speaking, processcmdbuff is only executed on data from Serial0 (ground or Avionics).  We expect that the TNC interface will be TNC0, so that the port address used by TNCattach will be 0 as well.  Is there any way for the RPi to ever assign the interface a different port address?
 
 I'm using KISS command 0xAA to identify data from Serial0 that's destined for Serial0 on the other end of the link.  0xA? is above the port enumeration range of TNCattach.

 for commands or responses bound for the other side, I'm adding a new command code back on.
 
 the command byte is unconditionally changed to 0xAA.  This works because processcmdbuff is ONLY run for packets from Serial0. We know that Serial0 is used are for local or remote commands only.
 
 Commands are structured to be simple (except perhaps for the testing support ones, which can do scans).  They have a generic format: ack, act, respond.
 
 it would be nice to synchronize this to the packet boundary of the databuffer.  However, there's no guarantee that there will be a complete packet in the databuffer when a complete command is received (they happen asynchronously)  
 we could wait for it to finish (by looking at the top of the databuffer and seeing if it's 0xC0, if not, we're not at a packet boundary) and then wait for it (most cycles around the loop are completed in the time to receive one byte via serial) or we could work backwards and try to insert it into the buffer (which would involve taking bytes off the stack until we get to a packet boundary, writing the packet and then putting the bytes back...yeeech)
 my original thought was that this was only needed for a halt, so it wasn't a big deal if we just trashed the last data packet
 HOWEVER, there's the question of doppler correction, which would be done by ground sending periodic frequency change commands.  they have to be processed in the context of a continuing data transfer, and it would be bad to trash packets.  So we gotta sync.
 I *think* the way to handle it is to ONLY process commands if the top of the databuff is 0xC0 or empty.  (check both).  It would effectively delay command processing, but only if data is coming in at the same time.  (net effect..none here, it gets taken care of in the main code, and HAS been implemented)
 
 note: commands have variable amounts of data, but these are generally some fixed amount per command. More generically, the data we receive after the first C0 and command code, and the last C0 is the length of the data in the command.

 */

#include "commands.h"


void Command::processcommand(CircularBuffer<byte, DATABUFFSIZE> &databuffer, Packet &commandpacket, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio, bool fault, FlashStorageClass<int> &operating_frequency)
{
    String response;
    Log.notice(F("processing the command \r\n"));
    Log.notice(F("commandcode: %X\r\n"), commandpacket.commandcode);
    switch (commandpacket.commandcode)
    {
    case 0x07: // beacon
    {
        if (commandpacket.packetlength != 6)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            beacon(commandpacket, watchdog, efuse, radio);
        }

        // no response
        break;
    }

    case 0x08: // manual antenna release
    {
        if (commandpacket.packetlength != 4)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            manual_antenna_release(commandpacket, watchdog, response);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x09: // status
    {
        if (commandpacket.packetlength != 3)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            status(efuse, radio, response, fault);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x0A: // reset
    {
        if (commandpacket.packetlength != 3)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            reset(databuffer, radio);
        }
        // no response
        break;
    }

    case 0x0B: // modify frequency
    {
        if (commandpacket.packetlength != 12)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            int new_freq = modify_frequency(commandpacket, radio, operating_frequency);
            response = String(new_freq);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x0C: // modify mode
    {
        if (commandpacket.packetlength != 4)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            modify_mode(commandpacket, radio);
            response = String(commandpacket.packetbody[0]);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x0D: // doppler frequencies
    {
        if (commandpacket.packetlength != 21)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            doppler_frequencies(commandpacket, radio, response);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x0E: // transmit callsign
    {
        if (commandpacket.packetlength != 3)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            transmit_callsign(databuffer);
        }
        // no response
        break;
    }

        // command 0x0F is sent by the Radio, not received.

    case 0x17: // transmit CW
    {
        if (commandpacket.packetlength != 5)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            transmitCW(commandpacket, radio, watchdog);
            response = "CW Mode complete";
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x18: // background RSSI
    {
        if (commandpacket.packetlength != 5)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            int result = background_rssi(commandpacket, radio, watchdog);
            response = String(result);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x19: // current RSSI
    {
        if (commandpacket.packetlength != 3)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            int result = current_rssi(radio);
            response = String(result, DEC);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x1A: // sweep transmitter
    {
        if (commandpacket.packetlength != 30)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            sweep_transmitter(commandpacket, radio, watchdog);
            response = "sweep complete, parked at original frequency";
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x1B: // sweep receiver
    {
        if (commandpacket.packetlength != 30)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            // int receiver_results[100];
            // int frequencies[100];
            sendACK(commandpacket.commandcode);
            response = "results sent to debug port"; // this is the only command where the response is sent before the action.
            sendResponse(commandpacket.commandcode, response);
            sweep_receiver(commandpacket, radio, watchdog);
        }
        break;
    }

    case 0x1C: // query radio register
    {
        if (commandpacket.packetlength != 6)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            uint16_t register_value = query_radio_register(commandpacket, radio);
            response = "Register Value (BIN): ";
            response += String(register_value, BIN);
            response += "Register Value (HEX): ";
            response += String(register_value, HEX);
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }

    case 0x1D: // adjust output power
    {
        if (commandpacket.packetlength != 4)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            adjust_output_power(commandpacket, radio);
            // adjust output power doesn't have a response!
            // response = (F("New power level is: " + String(result_float, 3));
            // sendResponse(commandpacket.commandcode, response);
            //il2p_testing();
        }
        break;
    }

    /*
    case 0x1E: // toggle frequency
    {
        if (commandpacket.packetlength != 3)
        {
            sendNACK(commandpacket.commandcode);
        }
        else
        {
            sendACK(commandpacket.commandcode);
            toggle_frequency(radio);
            response = "Frequency Toggle Test complete";
            sendResponse(commandpacket.commandcode, response);
        }
        break;
    }
    */

    default:
    {
        sendNACK(commandpacket.commandcode);
    }
    }
}

// command responses
void Command::sendACK(byte code)
{
    // create an ACK packet and send it out Serial0 - for testing at this moment just sent it to Serial
    // note that acks always go to Serial0
    Log.notice(F("ACK!!\r\n"));

    byte ackpacket[] = {0xC0, 0x00, 0x41, 0x43, 0x4B, 0x00, 0xC0}; // generic form of ack packet
    ackpacket[5] = code;                                           // replace code byte with the received command code

    Serial0.write(ackpacket, 7);
    Serial.write(ackpacket, 7);
}

void Command::sendNACK(byte code)
{
    // create an NACK packet and put it in the CMD TX queue
    // nacks always go to Serial0
    Log.notice(F("NACK!!\r\n"));                                          // bad code, no cookie!
    byte nackpacket[] = {0xC0, 0x00, 0x4E, 0x41, 0x43, 0x4B, 0x00, 0xC0}; // generic form of nack packet
    nackpacket[6] = code;                                                 // replace code byte with the received command code

    Serial0.write(nackpacket, 8);
    Serial.write(nackpacket, 8);
}

void Command::sendResponse(byte code, String &response)
{
    Log.notice(F("Sending the response\r\n"));

    // responses are KISS with cmd byte = 0x00, and always start with 'RES'
    byte responsestart[6]{0xC0, 0x00, 0x52, 0x45, 0x53, 0x00};
    responsestart[5] = code;
    byte responseend[1]{0xC0}; // placeholder for more if we need it

    byte responsebuff[200];                                 // create a buffer for the response bytes
    response.getBytes(responsebuff, response.length() + 1); // get the bytes

    // write it to Serial0 in parts
    // Serial0.write('\n');
    Serial0.write(responsestart, 6);                // first header
    Serial0.write(responsebuff, response.length()); // now the actual data
    Serial0.write(responseend, 1);                  // and finish the KISS packet

    // write it to Serial in parts
    Serial.write('\n');
    Serial.write(responsestart, 6);                // first header
    Serial.write(responsebuff, response.length()); // now the actual data
    Serial.write(responseend, 1);                  // and finish the KISS packet
}

void Command::beacon(Packet &commandpacket, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio)
{
    // Generate radio beacon character before the rest of the beacon, to average background RSSI levels
    // For now, only consider the S-meter level. Other error conditions will be added later
    // Written by isaac-silversat, 2024-07-30
    // Convert the S level to ASCII by adding 0x30
    byte beacondata[12]{};
    beacondata[10] = background_S_level(radio) + 0x30; // placeholder for radio status byte

    // If an error occurs, change the character set
    if OC5V
        beacondata[10] = beacondata[10] + 0x10;

    // Abbreviate S9
    if (beacondata[10] == '9')
        beacondata[10] = 'n'; // should be =, not comparison (==)

    // beaconstring consists of callsign (6 bytes), a space, and four beacon characters (4 bytes) + plus terminator (1 byte)
    memcpy(beacondata, constants::callsign, sizeof(constants::callsign));
    Log.trace(F("size of callsign %X\r\n"), sizeof(constants::callsign));
    beacondata[6] = 0x20; // add a space

    // copy in the beacon data from the cmdbuffer
    for (int i = 0; i < 3; i++)
    {                                                     // avionics is now only sending 3 status bytes (avionics, payload, eps)
        beacondata[i + 7] = commandpacket.packetbody[i]; // pull out the data bytes in the buffer (command data or response)
    }

    beacondata[11] = 0; // add null terminator
    int beaconstringlength = sizeof(beacondata);
    Log.trace(F("beacondata = %12c\r\n"), beacondata);

    sendbeacon(beacondata, beaconstringlength, watchdog, efuse, radio);
}

void Command::manual_antenna_release(Packet &commandpacket, ExternalWatchdog &watchdog, String &response)
{
    // I made the conscious decision not to check the efuse for this command.  I don't think we want to stop a burn once it's started.  However that's worth more discussion.
    // setup antenna object
    Antenna antenna(Release_A, Release_B);
    antenna.begin();

    // pull select byte
    char select = commandpacket.packetbody[0];
    if (select < 0x41 || select > 0x43)
    {
        sendNACK(commandpacket.commandcode);
    }
    else
    {
        // release the hounds!
        antenna.release(select, watchdog, response);
    }
}

void Command::status(Efuse &efuse, Radio &radio, String &response, bool fault)
{
    // act on command
    int reportlength = radio.reportstatus(response, efuse, fault); // the status should just be written to a string somewhere, or something like that.
    // Serial.println(response);

    // respond to command
    if (reportlength > 255)
    {
        response = "status string too long...go fix it";
    }
}

void Command::reset(CircularBuffer<byte, DATABUFFSIZE> &databuffer, Radio &radio)
{

#ifdef SILVERSAT_GROUND
    Log.notice(F("clearing the data buffer\r\n"));
    databuffer.clear();
    radio.clear_Radio_FIFO();
    // assuming for now that I don't need to clear the transmit buffer.  Need to verify this.
    Log.notice(F("resetting radio to receive state\r\n"));
    radio.dataMode();
#endif

    delay(3000); // this should cause the watchdog timer to fire off, resetting the system.  Otherwise it has no effect.
    // TODO: see if I need to set the transmit variable
    // transmit = false;
}

int Command::modify_frequency(Packet &commandpacket, Radio &radio, FlashStorageClass<int> &operating_frequency)
{
    // act on command
    int new_frequency = strtol(commandpacket.parameters[0].c_str(), NULL, 10);
    Log.notice(F("old frequency: %i\r\n"), radio.getTransmitFrequency());
    if (new_frequency < 400000000 || new_frequency > 525000000)
    {
        sendNACK(commandpacket.commandcode);
        return 0;
    }
    else
    {
        if (radio.getTransmitFrequency() == new_frequency)
        {
            // the requested frequency matches the one we're currently using, so we store it.
            operating_frequency.write(new_frequency);
        }
        radio.setTransmitFrequency(new_frequency);
        radio.setReceiveFrequency(new_frequency);
        Log.notice(F("new frequency: %i\r\n"), radio.getTransmitFrequency());
        return new_frequency;
    }
}

void Command::modify_mode(Packet &commandpacket, Radio &radio)
{
    // act on command
    char mode_index = commandpacket.packetbody[0];
    if (mode_index == 0x00)
    {
        radio.modulation = fsk_modulation;  //1200 baud.  this mode uses calculated receiver values
        radio.dataMode();
    }
    else if (mode_index == 0x01)
    {
        radio.modulation = gmsk_modulation;
        radio.dataMode();
    }
    else if (mode_index == 0x02)
    {
        radio.modulation = gmsk_modulation_il2p;
        radio.dataMode();
    }
    else
    {
        Log.error(F("ERROR: index out of bounds\r\n"));
        sendNACK(commandpacket.commandcode);
    }
    Log.trace(F("framing: %X\r\n"), radio.modulation.framing);
}

void Command::doppler_frequencies(Packet &commandpacket, Radio &radio, String &response)
{
    // act on command
    int transmit_frequency = strtol(commandpacket.parameters[0].c_str(), NULL, 10);
    Log.notice(F("transmit_frequency is: %i\r\n"), transmit_frequency);

    int receive_frequency = strtol(commandpacket.parameters[1].c_str(), NULL, 10);
    Log.notice(F("receive_frequency is: %i\r\n"), receive_frequency);
    
    if ((transmit_frequency < 400000000 || transmit_frequency > 525000000) || (receive_frequency < 400000000 || receive_frequency > 525000000))
    {
        sendNACK(commandpacket.commandcode);
    }
    else
    {
        radio.setTransmitFrequency(transmit_frequency);
        radio.setReceiveFrequency(receive_frequency);
        
        Log.trace(F("applied transmit frequency: %i\r\n"), radio.getTransmitFrequency());
        Log.trace(F("applied receive frequency: %i\r\n"), radio.getReceiveFrequency());

        // now update the frequency registers
        radio.setTransmitFrequency(transmit_frequency);
        radio.setReceiveFrequency(receive_frequency); 
        
        response =(String)(char *)commandpacket.packetbody;
    }
}

void Command::transmit_callsign(CircularBuffer<byte, DATABUFFSIZE> &databuffer)
{
    // act on command
    databuffer.push(constants::FEND);
    databuffer.push(0xAA);
    for (unsigned int i = 0; i < sizeof(constants::callsign) - 1; i++)
    {
        databuffer.push(constants::callsign[i]);
    }
    databuffer.push(constants::FEND);
    //il2p_testing();
}

void Command::transmitCW(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    int duration = strtol(commandpacket.parameters[0].c_str(), NULL, 10);
    
    if (duration <= 1)
    {
        duration = 1;
    }
    else if (duration >= 99)
    {
        duration = 99;
    }

    Log.notice(F("duration: %d\r\n"), duration);
    radio.cwMode(duration, watchdog);
}

int Command::background_rssi(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    // dwell time per step
    
    unsigned long integrationtime = strtol(commandpacket.parameters[0].c_str(), NULL, 10);

    if (integrationtime < 1)
    {
        integrationtime = 1;
    }

    Log.notice(F("integration time: %d\r\n"), integrationtime);
    unsigned long starttime = millis();
    unsigned int rssi_sum{0};
    // byte rssi;
    unsigned int count{0};

    do
    {
        rssi_sum += radio.rssi(); // just keep summing up readings as an unsigned 32 bit value
        count++;                      // and keep a count of how many times you did it
        delay(100);                   // let's arbitrarily delay 100mS between readings, so about number of readings is about 10x the integration time
        watchdog.trigger();
    } while ((millis() - starttime) < integrationtime * 1000);

    unsigned int background_rssi = rssi_sum / count;
    Log.notice(F("background rssi: %d\r\n"), background_rssi);
    Log.trace(F("rssi sum: %d\r\n"), rssi_sum);
    Log.trace(F("count: %d\r\n"), count);

    return background_rssi;
}

// Get the background RSSI as an S-meter reading by scaling the range, assuming RSSI is in dBm.
// Written by isaac-silversat, 2024-07-30
// Uses values from Wikipedia (https://en.wikipedia.org/wiki/S_meter); please verify somewhere else!
char Command::background_S_level(Radio &radio)
{
    // Get the background RSSI
    int RSSI{current_rssi(radio)}; 
    // isaac, the rough equation that relates the return value (expressed as a decimal) to the received power is:
    //  received_power = 0.961 * return_value - 264
    // correcting scope of variable S_level & changing to assignment from initializer
    char S_level{0};

    // For the purposes of the beacon, ensure the S-meter level is between 0 and 9 (including endpoints)
    if (RSSI < -121)
        S_level = 0;
    else if (RSSI > -73)
        S_level = 9;
    else
    {
        // The relationship between dBm and RSSI is linear from S1 to S9, so use linear scaling.
        // Model: y = mx + b
        // m = (S9-S1)/(-73-121 dBm)=8/48 = 1/6
        // Using S9, 9 = -73m + b => b = 9 + 73m = 127/6
        const double B{127 / 6};
        S_level = RSSI / 6 + B;
    }

    return S_level;
}

int Command::current_rssi(Radio &radio)
{
    // act on command
    Log.trace(F("current selected synth for RSSI: %X\r\n"), radio.getSynth());  //debug message, so letting this one remain as an ax call.
    uint8_t rssi = radio.rssi();

    return rssi;
}

void Command::sweep_transmitter(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    // get the parameters
    // frequency is part of the ax_config structure
    // start frequency
    int original_frequency = radio.getTransmitFrequency();

    int startfreq = strtol(commandpacket.parameters[0].c_str(), NULL, 10);
    int stopfreq = strtol(commandpacket.parameters[1].c_str(), NULL, 10);
    int numsteps = strtol(commandpacket.parameters[2].c_str(), NULL, 10);
    int dwelltime = strtol(commandpacket.parameters[3].c_str(), NULL, 10);
    int stepsize = (int)((stopfreq - startfreq) / numsteps); // find the closest integer to the step size
    Log.notice(F("stepsize = %d\r\n"), stepsize);
    // implement some rudimentary checks
    if (startfreq < 400000000)
        startfreq = 400000000; // it's too low go to the bottom of the range
    if (startfreq > 525000000)
        startfreq = 400000000; // it's too high go to the bottom of the range
    if (stopfreq < 400000000)
        stopfreq = 525000000; // it's too low go to the top of the range
    if (stopfreq > 525000000)
        stopfreq = 525000000; // it's too high go to the top of the range
    if (numsteps < 1)
        numsteps = 1;
    if (numsteps > 999)
        numsteps = 999;
    if (dwelltime < 10)
        dwelltime = 10;
    if (dwelltime > 999)
        dwelltime = 999;

    radio.setTransmitFrequency(startfreq);

    // beaconMode handles the state of the T/R switch
    radio.beaconMode();

    for (int j = startfreq; j <= stopfreq; j += stepsize)
    {
        Log.notice(F("current frequency: %i\r\n"), j);
        while (radio.setTransmitFrequency(j) != AX_INIT_OK)
            ; // sweeps can have steps much wider than what can be done with ax_force_quick_adjust_freq_A
        // if ax_adjust_frequency has to range, then it leaves synth B enabled.
        radio.setSynthA();
        // start transmitting
        Log.notice(F("output for %d milliseconds\r\n"), dwelltime);
        digitalWrite(PAENABLE, HIGH);
        digitalWrite(PIN_LED_TX, HIGH);
        digitalWrite(AX5043_DATA, HIGH);

        delay(dwelltime);

        // stop transmitting
        digitalWrite(AX5043_DATA, LOW);
        digitalWrite(PAENABLE, LOW); // turn off the PA
        digitalWrite(PIN_LED_TX, LOW);
        Log.notice(F("done\r\n"));

        watchdog.trigger(); // trigger the external watchdog after each frequency
    }
    // drop out of wire mode
    radio.dataMode();
    radio.setTransmitFrequency(original_frequency);
}

int Command::sweep_receiver(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    
    // we use sythesizer B for receive
    int original_frequency = radio.getReceiveFrequency();

    int startfreq = strtol(commandpacket.parameters[0].c_str(), NULL, 10);
    int stopfreq = strtol(commandpacket.parameters[1].c_str(), NULL, 10);
    int numsteps = strtol(commandpacket.parameters[2].c_str(), NULL, 10);
    int dwelltime = strtol(commandpacket.parameters[3].c_str(), NULL, 10);
    int stepsize = (int)((stopfreq - startfreq) / numsteps); // find the closest integer to the step size
    Log.notice(F("stepsize = %d\r\n"), stepsize);

    // implement some rudimentary checks
    if (startfreq < 400000000)
        startfreq = 400000000; // it's too low go to the bottom of the range
    if (startfreq > 525000000)
        startfreq = 400000000; // it's too high go to the bottom of the range
    if (stopfreq < 400000000)
        stopfreq = 525000000; // it's too low go to the top of the range
    if (stopfreq > 525000000)
        stopfreq = 525000000; // it's too high go to the top of the range
    if (numsteps < 1)
        numsteps = 1;
    if (numsteps > 999)
        numsteps = 999;
    if (dwelltime < 10)
        dwelltime = 10;
    if (dwelltime > 999)
        dwelltime = 999;

    radio.setReceiveFrequency(startfreq);
    radio.setReceive();

    for (int j = startfreq; j <= stopfreq; j += stepsize)
    {
        Log.notice(F("current frequency: %d\r\n"), j);
        radio.setReceiveFrequency(j);
        Log.trace(F("(after adjust) current selected synth for Rx: %X\r\n"), radio.getSynth()); 

        // start requesting RSSI samples
        Log.notice(F("measuring for %d milliseconds\r\n"), dwelltime);
        unsigned int starttime = millis();
        int samples{0};
        unsigned int rssi_total{0};

        delay(1); // seeing if a slight delay helps get the first sample right.  YES, it does!
        do
        {
            uint8_t rssi = radio.rssi();
            rssi_total += rssi;
            Log.trace(F("sample %X: %X\r\n"), samples, rssi);
            samples++;
            delay(50); // this is a guess for now.  I don't know how often you can reasonably query the RSSI
        } while (millis() - starttime < dwelltime);

        int integrated_rssi = rssi_total / samples; // intentional integer division.  I want to return the rounded down average rssi.
        /*
        while (millis() - starttime < dwelltime)
        {
            byte rssi = radio.rssi();
            Log.tracve(F("sample %X: %X\r\n"), samples, rssi);
            integrated_rssi = (integrated_rssi*(samples-1)+rssi)/(samples);
            samples++;
            delay(50); //this is a guess for now.  I don't know how often you can reasonably query the RSSI
        }
        */
        Log.notice(F("number of samples: %i\r\n"), samples);
        Log.notice(F("frequency: %d, rssi: %d\r\n"), j, integrated_rssi);
        watchdog.trigger(); // trigger the external watchdog after each frequency
    }

    // return to the original frequency
    radio.setReceiveFrequency(original_frequency);
    // return the number of samples
    return numsteps;
}

//this command is for debugging, so I'm not worrying about the direct ax calls
uint16_t Command::query_radio_register(Packet &commandpacket, Radio &radio)
{
    // act on command
    char ax5043_register[6];
    for (int i = 0; i < 5; i++)
    {
        ax5043_register[i] = commandpacket.packetbody[i];
    }
    ax5043_register[5] = 0;
    int ax5043_register_int = strtol(ax5043_register, NULL, 16);
    Log.notice(F("register: %X\r\n"), ax5043_register_int);

    uint16_t register_value = radio.getRegValue(ax5043_register_int);
    Log.notice(F("register value: %X\r\n"), register_value);
    return register_value;
}

float Command::adjust_output_power(Packet &commandpacket, Radio &radio)
{
    // act on command
    unsigned char power = commandpacket.packetbody[0];
    float power_frac{0};

    if (power == 0x0A || power > 0x0A)
    {
        radio.modulation.power = 1;
        power_frac = 1;
    }
    else
    {
        power_frac = (float(power) * 10) / 100;
        radio.modulation.power = power_frac;
    }
    Log.notice(F("new power level is: %d\r\n"), power);
    Log.notice(F("new power fraction is: %f\r\n"), power_frac);
    // now reload the configuration into the AX5043
    radio.dataMode();
    Log.notice(F("receiver on\r\n"));

    return power_frac;
}

//this command may be depricated soon
/*
void Command::toggle_frequency(Radio &radio)
{
    // act on command
    // this builds on the CW command, but toggles between the two frequency registers to
    // allows measuring of the PLL dynamic behavior
    // you need to set frequency B to something discernably different that frequency A!

    ax_init(&radio.config); // do an init first
    // modify the power to match what's in the modulation structure...make sure the modulation type matches
    ask_modulation.power = radio.modulation.power;

    ax_default_params(&radio.config, &ask_modulation); // load the RF parameters

    pinfunc_t func = 0x84;              // set for wire mode
    ax_set_pinfunc_data(&radio.config, func); // remember to set this back when done!

    // set the RF switch to transmit
    radio.setTransmit();

    // start transmitting
    int duration = 2;
    Log.trace(F("output CW for %d seconds ", duration);
    digitalWrite(PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(PIN_LED_TX, HIGH);
    digitalWrite(AX5043_DATA, HIGH);

    for (int i = 0; i < 10; i++)
    {
        delay(duration * 1000);
        ax_TOGGLE_SYNTH(&radio.config);
    }
    // should be back on A?  (maybe toggling isn't such a hot idea, lol)

    // stop transmitting
    digitalWrite(AX5043_DATA, LOW);
    digitalWrite(PAENABLE, LOW); // turn off the PA
    digitalWrite(PIN_LED_TX, LOW);
    Log.trace(F("done ");

    // drop out of wire mode
    func = 2;
    ax_set_pinfunc_data(&radio.config, func);

    radio.dataMode();
    // now put it back the way you found it.
    
    //ax_init(&radio.config);                        // do a reset
    //ax_default_params(&radio.config, &radio.modulation); // ax_modes.c for RF parameters
    //Log.trace(F("default params loaded ");
    // Serial.println(F("default params loaded ");
    //ax_rx_on(&radio.config, &radio.modulation);

    Log.trace(F("receiver on ");
    // Serial.println(F("receiver on ");
}

*/