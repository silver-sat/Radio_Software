/**
 * @file commands.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2024-07-19
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

// #include "ax.h"
#include "commands.h"
// #include "constants.h"

bool Command::processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, packet &packet)
{
    // first remove the seal... 0xC0
    cmdbuffer.shift();
    // and then grab the command code
    packet.commandcode = cmdbuffer.shift();
    debug_printf("command code is: %x \r\n", packet.commandcode);

    if (packet.commandcode == 0xAA || packet.commandcode == 0x00) {
        // nothing to see here, it's not for me...forward to the other end, so copy this over to the tx buffer
        databuffer.push(constants::FEND);
        // so for commands or responses bound for the other side, I'm adding a new command code back on to indicate where it's going.
        databuffer.push(0xAA);
        // you're starting at the second byte of the total packet
        //noInterrupts();  //turn off interrupts until this is done.  This is to avoid writing to the buffer until all the packet is shifted out.
        for (int i = 2; i < packetlength; i++)
        {
            // shift it out of cmdbuffer and push it into databuffer, don't need to push a final 0xC0 because it's still part of the packet
            databuffer.push(cmdbuffer.shift());
        }
        //interrupts();
        debug_printf("packetlength = %i \r\n", packetlength);  //the size of the packet
        debug_printf("databuffer length = %i \r\n", databuffer.size()); //the size that was pushed into the databuffer
        return false;
    }
    else {
        //it's possibly a local command
        for (int i=2; i<(packetlength-1); i++)  //in this case we don't want the last C0
        {
            packet.commandbody[i] = cmdbuffer.shift();
        }
        packet.commandbody[packetlength-2] = 0;  //put a null in the next byte...if the command has no body (length =3), then it puts a null in the first byte
        cmdbuffer.shift(); //remove the last C0 from the buffer
        debug_printf("command body: %20x \r\n", packet.commandbody);
        return true;
    }   
}

void Command::processcommand(CircularBuffer<byte, DATABUFFSIZE> &databuffer, packet &commandpacket, ax_config &config, ax_modulation &modulation, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio, bool fault)
{
    String response;
    debug_printf("commandcode: %x \r\n", commandpacket.commandcode);
    switch (commandpacket.commandcode) {
        case 0x07:  //beacon
        {
            sendACK(commandpacket.commandcode);
            beacon(commandpacket, config, modulation, watchdog, efuse, radio);
            //no response
            break;
        }

        case 0x08:  //manual antenna release
        {
            sendACK(commandpacket.commandcode);
            manual_antenna_release(commandpacket, watchdog, response);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x09:  //status
        {
            sendACK(commandpacket.commandcode);
            status(commandpacket, config, modulation, efuse, radio, response, fault);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x0A: //reset
        {
            sendACK(commandpacket.commandcode);
            reset(databuffer, modulation, commandpacket, config, radio);
            //no response
            break;
        }

        case 0x0B: //modify frequency
        {
            sendACK(commandpacket.commandcode);
            int newfreq = modify_frequency(commandpacket, config);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x0C: //modify mode
        {
            sendACK(commandpacket.commandcode);
            modify_mode(commandpacket, config, modulation);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x0D: //doppler frequencies
        {
            sendACK(commandpacket.commandcode);
            doppler_frequencies(commandpacket, config, modulation);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x0E: //transmit callsign
        {
            sendACK(commandpacket.commandcode);
            transmit_callsign(databuffer);
            //no response
            break;
        }

        case 0x17: //transmit CW
        {
            sendACK(commandpacket.commandcode);
            transmitCW(commandpacket, config, modulation, radio, watchdog);
            response = "CW Mode complete";
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x18: //background RSSI
        {
            sendACK(commandpacket.commandcode);
            int result = background_rssi(commandpacket, config, modulation, radio, watchdog);
            response = String(result);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x19: //current RSSI
        {
            sendACK(commandpacket.commandcode);
            int result = current_rssi(commandpacket, config);
            response = String(result, DEC);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x1A: //sweep transmitter
        {
            sendACK(commandpacket.commandcode);
            sweep_transmitter(commandpacket, config, modulation, radio, watchdog);
            response = "sweep complete, parked at original frequency";
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x1B: //sweep receiver
        {
            sendACK(commandpacket.commandcode);
            sweep_receiver();
            response = "not yet implemented";
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x1C: //query radio register
        {
            sendACK(commandpacket.commandcode);
            uint16_t register_value = query_radio_register(commandpacket, config);
            response = "Register Value (BIN): ";
            response += String(register_value, BIN);
            response += "Register Value (HEX): ";
            response += String(register_value, HEX);
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x1D: //adjust output power
        {
            sendACK(commandpacket.commandcode);
            float result_float = adjust_output_power(commandpacket, config, modulation);
            response = ("New power level is: " + String(result_float, 3));
            sendResponse(commandpacket.commandcode, response);
            break;
        }

        case 0x1E: //toggle frequency
        {
            sendACK(commandpacket.commandcode);
            toggle_frequency(config, modulation, radio);
            response = "Frequency Toggle Test complete";
            sendResponse(commandpacket.commandcode, response);
            break;
        }

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
    debug_printf("ACK!! \r\n");

    byte ackpacket[] = {0xC0, 0x00, 0x41, 0x43, 0x4B, 0x00, 0xC0}; // generic form of ack packet
    ackpacket[5] = code;                                           // replace code byte with the received command code

    Serial0.write(ackpacket, 7);
    Serial.write(ackpacket, 7);
}

void Command::sendNACK(byte code)
{
    // create an NACK packet and put it in the CMD TX queue
    // nacks always go to Serial0
    debug_printf("NACK!! \r\n");                                          // bad code, no cookie!
    byte nackpacket[] = {0xC0, 0x00, 0x4E, 0x41, 0x43, 0x4B, 0x00, 0xC0}; // generic form of nack packet
    nackpacket[6] = code;                                                 // replace code byte with the received command code

    Serial0.write(nackpacket, 8);
    Serial.write(nackpacket, 8);
}

void Command::sendResponse(byte code, String &response)
{
    debug_printf("Sending the response \r\n");

    // responses are KISS with cmd byte = 0x00, and always start with 'RES'
    byte responsestart[6]{0xC0, 0x00, 0x52, 0x45, 0x53, 0x00};
    responsestart[5] = code;
    byte responseend[1]{0xC0}; // placeholder for more if we need it

    byte responsebuff[200];                                 // create a buffer for the response bytes
    response.getBytes(responsebuff, response.length() + 1); // get the bytes

    // write it to Serial0 in parts
    Serial0.write('\n');
    Serial0.write(responsestart, 6);                // first header
    Serial0.write(responsebuff, response.length()); // now the actual data
    Serial0.write(responseend, 1);                  // and finish the KISS packet

    // write it to Serial in parts
    Serial.write('\n');
    Serial.write(responsestart, 6);                // first header
    Serial.write(responsebuff, response.length()); // now the actual data
    Serial.write(responseend, 1);                  // and finish the KISS packet
}

void Command::beacon(packet &commandpacket, ax_config &config, ax_modulation &modulation, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio)
{    
    // beaconstring consists of callsign (6 bytes), a space, and four beacon characters (4 bytes) + plus terminator (1 byte)
    byte beacondata[12]{};
    memcpy(beacondata, constants::callsign, sizeof(constants::callsign));
    debug_printf("size of callsign %x \r\n", sizeof(constants::callsign));
    beacondata[6] = 0x20; // add a space

    // copy in the beacon data from the cmdbuffer
    for (int i = 0; i < 3; i++)
    {                                      // avionics is now only sending 3 status bytes (avionics, payload, eps)
        beacondata[i+7] = commandpacket.commandbody[i]; // pull out the data bytes in the buffer (command data or response)
    }

    beacondata[10] = 0x45; // placeholder for radio status byte
    beacondata[11] = 0;    // add null terminator
    int beaconstringlength = sizeof(beacondata);
    debug_printf("beacondata = %12c \r\n", beacondata);

    sendbeacon(beacondata, beaconstringlength, config, modulation, watchdog, efuse, radio);
}

void Command::manual_antenna_release(packet &commandpacket, ExternalWatchdog &watchdog, String &response)
{
    // I made the conscious decision not to check the efuse for this command.  I don't think we want to stop a burn once it's started.  However that's worth more discussion.
    // setup antenna object
    Antenna antenna(Release_A, Release_B);
    antenna.begin();

    // pull select byte
    char select = commandpacket.commandbody[0];

    // release the hounds!
    antenna.release(select, watchdog, response);
}

void Command::status(packet &commandpacket, ax_config &config, ax_modulation &modulation, Efuse &efuse, Radio &radio, String &response, bool fault)
{
    // act on command
    int reportlength = radio.reportstatus(response, config, modulation, efuse, fault); // the status should just be written to a string somewhere, or something like that.
    //Serial.println(response);

    // respond to command
    if (reportlength > 200)
    {
        response = "status string too long...go fix it"; 
    }    
}

void Command::reset(CircularBuffer<byte, DATABUFFSIZE> &databuffer, ax_modulation &modulation, packet &commandpacket, ax_config &config, Radio &radio)
{
    debug_printf("clearing the data buffer \r\n");
    databuffer.clear();

    debug_printf("clearing the AX5043 FIFO"); // may be unnecessary...may have unintended consequences?
    ax_fifo_clear(&config);

    // assuming for now that I don't need to clear the transmit buffer.  Need to verify this.
    debug_printf("resetting radio to receive state \r\n");
    // ax_init(&config);  // this does a reset, so needs to be first
    // ax_default_params(&config, &modulation);  // load the current RF modulation parameters for the current config
    // ax_rx_on(&config, &modulation);
    radio.dataMode(config, modulation);

    //TODO: see if I need to set the transmit variable
    //transmit = false;
}

int Command::modify_frequency(packet &commandpacket, ax_config &config)
{
    // act on command
    char freqstring[10];
    for (int i = 0; i < 9; i++)
    {
        freqstring[i] = commandpacket.commandbody[i]; // pull out the data bytes in the buffer (command data or response)
    }
    freqstring[9] = 0;
    // convert string to integer, modify config structure and implement change on radio
    // I believe the function call updates the config.
    debug_printf("old frequency: %i \r\n", config.synthesiser.A.frequency);
    ax_adjust_frequency_A(&config, atoi(freqstring));
    ax_adjust_frequency_B(&config, atoi(freqstring));
    debug_printf("new frequency: %i \r\n", config.synthesiser.A.frequency);
    // config.synthesiser.A.frequency = atoi(freqstring);
    // config.synthesiser.B.frequency = atoi(freqstring);
    return atoi(freqstring);
}

void Command::modify_mode(packet &commandpacket, ax_config &config, ax_modulation &modulation)
{
    // act on command
    char mode_index = commandpacket.commandbody[0];
    if (mode_index == 0x00)
    {
        modulation.fec = 0;
        modulation.encoding = AX_ENC_NRZI;
        modulation.shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED;
        modulation.bitrate = 9600;
        modulation.continuous = 0;
        ax_init(&config); // this does a reset, so needs to be first
        // load the RF parameters for the current config
        ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
        ax_rx_on(&config, &modulation);
    }
    else if (mode_index == 0x01)
    {
        modulation.fec = 0;
        modulation.encoding = AX_ENC_NRZI;
        modulation.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
        modulation.bitrate = 9600;
        modulation.continuous = 0;
        ax_init(&config); // this does a reset, so needs to be first
        // load the RF parameters for the current config
        ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
        ax_rx_on(&config, &modulation);
    }
    else if (mode_index == 0x02)
    {
        // ax_MODIFY_FEC(&config, &modulation, true);
        // ax_MODIFY_SHAPING(&config, &modulation, 1);
        modulation.fec = 1;
        modulation.encoding = AX_ENC_NRZ;
        modulation.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
        modulation.bitrate = 19200;
        modulation.continuous = 0;
        ax_init(&config); // this does a reset, so needs to be first
        // load the RF parameters for the current config
        ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
        ax_rx_on(&config, &modulation);
    }
    else
    {
        debug_printf("ERROR: index out of bounds \r\n");
    }
}

void Command::doppler_frequencies(packet &commandpacket, ax_config &config, ax_modulation &modulation)
{
    // act on command
    // this grabs the value from the command and updates the
    // the offset needs to be applied in the main program
    char transmit_frequency_string[9];
    char receive_frequency_string[9];
    //TODO: see if I can't just do "atoi(commandpacket.commandbody)" or do I need the substring?
    for (int i = 0; i < 9; i++)
    {
        transmit_frequency_string[i] = (char)commandpacket.commandbody[i];
    }
    int transmit_frequency = atoi(transmit_frequency_string);
    debug_printf("transmit_frequency is: %i \r\n", transmit_frequency);
    
    for (int i = 9; i < 18; i++)
    {
        receive_frequency_string[i] = (char)commandpacket.commandbody[i];
    }
    int receive_frequency = atoi(receive_frequency_string);
    debug_printf("receive_frequency is: %i \r\n", receive_frequency);

    config.synthesiser.A.frequency = transmit_frequency;
    config.synthesiser.B.frequency = receive_frequency;

    debug_printf("applied transmit frequency: %i \r\n", config.synthesiser.A.frequency);
    debug_printf("applied receive frequency: %i \r\n", config.synthesiser.B.frequency);

    // now update the frequency registers
    ax_adjust_frequency_A(&config, transmit_frequency);
    ax_adjust_frequency_B(&config, receive_frequency);
}

void Command::transmit_callsign(CircularBuffer<byte, DATABUFFSIZE> &databuffer)
{
    // act on command
    databuffer.push(constants::FEND);
    databuffer.push(0xAA);
    for (int i = 0; i < sizeof(constants::callsign) - 1; i++)
    {
        databuffer.push(constants::callsign[i]);
    }
    databuffer.push(constants::FEND);
}

void Command::transmitCW(packet &commandpacket, ax_config &config, ax_modulation &modulation, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    char durationstring[3];
    durationstring[0] = (char)commandpacket.commandbody[0];
    durationstring[1] = (char)commandpacket.commandbody[1];
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

    radio.cwMode(config, modulation, duration, watchdog);
}

int Command::background_rssi(packet &commandpacket, ax_config &config, ax_modulation &modulation, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    // dwell time per step
    char integrationtime_string[3]; // to hold the number of steps (3 bytes + null)
    for (int i = 0; i < 2; i++)
    {
        integrationtime_string[i] = (char)commandpacket.commandbody[i]; // pull out the data bytes in the buffer (command data or response)
    }
    integrationtime_string[2] = 0; // set the terminator
    debug_printf("integration time: %s \r\n", integrationtime_string);

    unsigned long integrationtime = (unsigned long)atoi(integrationtime_string);
    unsigned long starttime = millis();
    int rssi_sum{0};
    // byte rssi;
    unsigned long count{0};

    do
    {
        rssi_sum += ax_RSSI(&config); // just keep summing up readings as an unsigned 32 bit value
        count++;                      // and keep a count of how many times you did it
        delay(100);                   // let's arbitrarily delay 100mS between readings, so about number of readings is about 10x the integration time
        watchdog.trigger();
    } while ((millis() - starttime) < integrationtime * 1000);

    int background_rssi = rssi_sum / count;
    debug_printf("background rssi: %u \r\n", background_rssi);
    debug_printf("rssi sum: %u \r\n", rssi_sum);
    debug_printf("count: %lu \r\n", count);

    return background_rssi;
}

int Command::current_rssi(packet &commandpacket, ax_config &config)
{
    // act on command
    byte rssi = ax_RSSI(&config);
    
    return rssi;
}

void Command::sweep_transmitter(packet &commandpacket, ax_config &config, ax_modulation &modulation, Radio &radio, ExternalWatchdog &watchdog)
{
    // act on command
    // get the parameters
    // frequency is part of the ax_config structure
    // start frequency
    int original_frequency = config.synthesiser.A.frequency;
    char startfreqstring[10]; // to hold the beacon data (9 bytes + null)
    for (int i = 0; i < 9; i++)
    {
        startfreqstring[i] = (char)commandpacket.commandbody[i]; // pull out the data bytes in the buffer (command data or response)
    }
    startfreqstring[9] = 0; // set the terminator
    debug_printf("start frequency: %s \r\n", startfreqstring);
    // stop frequency
    char stopfreqstring[10]; // to hold the beacon data (9 bytes + null)
    for (int i = 0; i < 9; i++)
    {
        stopfreqstring[i] = (char)commandpacket.commandbody[i+10]; // allow one for the space (9 +1)
    }
    stopfreqstring[9] = 0; // set the terminator
    debug_printf("stop frequency: %s \r\n", stopfreqstring);
    // number of steps
    char numberofstepsstring[4]; // to hold the number of steps (3 bytes + null)
    for (int i = 0; i < 3; i++)
    {
        numberofstepsstring[i] = (char)commandpacket.commandbody[i+20]; // allow two for two spaces (9+9 +2)
    }
    numberofstepsstring[3] = 0; // set the terminator
    debug_printf("number of steps: %s \r\n", numberofstepsstring);
    // dwell time per step
    char dwellstring[4]; // to hold the number of steps (3 bytes + null)
    for (int i = 0; i < 3; i++)
    {
        dwellstring[i] = (char)commandpacket.commandbody[i+24]; // allow three for three spaces (9+9+3 +3)
    }
    dwellstring[3] = 0; // set the terminator
    debug_printf("dwell time: %s \r\n", dwellstring);

    int startfreq = atoi(startfreqstring);
    int stopfreq = atoi(stopfreqstring);
    int numsteps = atoi(numberofstepsstring);
    int dwelltime = atoi(dwellstring);
    int stepsize = (int)((stopfreq - startfreq) / numsteps); // find the closest integer to the step size
    debug_printf("stepsize = %u \r\n", stepsize);

    config.synthesiser.A.frequency = startfreq;
    // config.synthesiser.B.frequency = startfreq;

    radio.beaconMode(config, ask_modulation);

    for (int j = startfreq; j <= stopfreq; j += stepsize)
    {
        debug_printf("current frequency: %u \r\n", j);
        ax_force_quick_adjust_frequency_A(&config, j);

        //TODO: look into converting the dit/dah to a generic single command with time as a parameter.  Could make the generic and derive dit/dah from that for clarity.
        // start transmitting
        debug_printf("output for %u milliseconds \r\n", dwelltime);
        digitalWrite(PAENABLE, HIGH);
        digitalWrite(PIN_LED_TX, HIGH);
        digitalWrite(AX5043_DATA, HIGH);

        delay(dwelltime);

        // stop transmitting
        digitalWrite(AX5043_DATA, LOW);
        digitalWrite(PAENABLE, LOW); // turn off the PA
        digitalWrite(PIN_LED_TX, LOW);
        debug_printf("done \r\n");

        watchdog.trigger(); // trigger the external watchdog after each frequency
    }
    // drop out of wire mode
    radio.dataMode(config, modulation);
    ax_adjust_frequency_A(&config, original_frequency);
}

void Command::sweep_receiver()
{
    // act on command
    // PLACEHOLDER
}

uint16_t Command::query_radio_register(packet &commandpacket, ax_config &config)
{
    // act on command
    char ax5043_register[6];
    for (int i = 0; i < 5; i++)
    {
        ax5043_register[i] = commandpacket.commandbody[i];
    }
    ax5043_register[5] = 0;
    int ax5043_register_int = atoi(ax5043_register);
    
    uint16_t register_value = ax_hw_read_register_8(&config, ax5043_register_int);
    return register_value;
}

float Command::adjust_output_power(packet &commandpacket, ax_config &config, ax_modulation &modulation)
{
    // act on command
    char power_str[4];
    for (int i = 0; i < 3; i++)
    {
        power_str[i] = commandpacket.commandbody[i];
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
    float power_frac = float(power) / 100.0;
    modulation.power = power_frac;
    debug_printf("new power level is: %d \r\n", power);
    debug_printf("new power fraction is: %f \r\n", power_frac);
    // now reload the configuration into the AX5043
    ax_init(&config);                        // do a reset
    ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
    debug_printf("default params loaded \r\n");
    debug_printf("power level: %f \r\n", modulation.power);
    // Serial.println("default params loaded \r\n");
    ax_rx_on(&config, &modulation);
    debug_printf("receiver on \r\n");    

    return power_frac;
}

void Command::toggle_frequency(ax_config &config, ax_modulation &modulation, Radio &radio)
{
    // act on command
    // this builds on the CW command, but toggles between the two frequency registers to
    // allows measuring of the PLL dynamic behavior
    // you need to set frequency B to something discernably different that frequency A!

    ax_init(&config); // do an init first
    // modify the power to match what's in the modulation structure...make sure the modulation type matches
    ask_modulation.power = modulation.power;

    ax_default_params(&config, &ask_modulation); // load the RF parameters

    pinfunc_t func = 0x84;              // set for wire mode
    ax_set_pinfunc_data(&config, func); // remember to set this back when done!

    // set the RF switch to transmit
    radio.setTransmit(config, ask_modulation);

    // start transmitting
    int duration = 2;
    debug_printf("output CW for %u seconds \r\n", duration);
    digitalWrite(PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(PIN_LED_TX, HIGH);
    digitalWrite(AX5043_DATA, HIGH);

    for (int i = 0; i < 10; i++)
    {
        delay(2000);
        ax_TOGGLE_SYNTH(&config);
    }
    // should be back on A?  (maybe toggling isn't such a hot idea, lol)

    // stop transmitting
    digitalWrite(AX5043_DATA, LOW);
    digitalWrite(PAENABLE, LOW); // turn off the PA
    digitalWrite(PIN_LED_TX, LOW);
    debug_printf("done \r\n");

    // drop out of wire mode
    func = 2;
    ax_set_pinfunc_data(&config, func);

    // now put it back the way you found it.
    ax_init(&config);                        // do a reset
    ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
    debug_printf("default params loaded \r\n");
    // Serial.println("default params loaded \r\n");
    ax_rx_on(&config, &modulation);
    debug_printf("receiver on \r\n");
    // Serial.println("receiver on \r\n");
}