/**
 * @file commands.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2024-07-24
 *
 * The packet class is also defined here.  It's pretty simple, just the command code and the command body.
 * We could make it more complex if that's needed.
 *
 * The command class includes the functions to parse the buffers for complete packets (processcmdbuff()) and for
 * processing the local commands (processcommand())
 *
 */


#ifndef COMMANDS_H
#define COMMANDS_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512 // 4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 // 32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include "beacon.h"
#include "KISS.h"
#include "constants.h"
#include "ax.h"
#include "ax_modes.h"
#include "ax_hw.h"
#include "ExternalWatchdog.h"
#include "efuse.h"
#include "radio.h"
#include "antenna.h"
#include "packet.h"

#include <LibPrintf.h>
#include <SPI.h>
#include <Wire.h>
#include <CircularBuffer.hpp>
#include <ArduinoLog.h>

//#include <cstdlib> //for atoi function, may replace this with String functions, but it's working...
#include <FlashStorage.h>
#include <Temperature_LM75_Derived.h>

#include "Arduino.h"

class Command
{
public:
    bool processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, Packet &commandpacket);                                                             // determines if packet is destined for other end of link, and if not, extracts the body
    void processcommand(CircularBuffer<byte, DATABUFFSIZE> &databuffer, Packet &commandpacket, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio, bool fault, FlashStorageClass<int> &operating_frequency); // calls the appropriate command based on the command code

private:
    String response;
    Packet packet;

    // command responses
    void sendACK(byte code);
    void sendNACK(byte code);
    void sendResponse(byte code, String &response);

    // commands

    // function arguments:
    // &commandpacket holds structure containing body of the command
    // &config is the radio configuration, most AX commands need this
    // &modulation holds the current modulation configuration (GFSK vs. ASK generally)
    // &watchdog is needed if the command may cause long enough of a delay to trip the watchdog.  e.g. beacons
    // &radio is needed if you want to query or change the radio state
    // &response is the String that holds the response.  Only some commands have a response.
    // &databuffer is needed if you want to push a packet across the RF link (e.g. send callsign)
    // &efuse is the efuse class instance, needed to make queries of current and status
    // &operating_frequency is the current default operating frequency stored in the internal flash of the SAMD21

    void beacon(Packet &commandpacket, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio);
    void manual_antenna_release(Packet &commandpacket, ExternalWatchdog &watchdog, String &response);
    void status(Efuse &efuse, Radio &radio, String &response, bool fault);
    void reset(CircularBuffer<byte, DATABUFFSIZE> &databuffer, Radio &radio);
    int modify_frequency(Packet &commandpacket, Radio &radio, FlashStorageClass<int> &operating_frequency);
    void modify_mode(Packet &commandpacket, Radio &radio);
    void doppler_frequencies(Packet &commandpacket, Radio &radio, String &response);
    void transmit_callsign(CircularBuffer<byte, DATABUFFSIZE> &databuffer);
    // reset_5V() is handled in the efuse class
    void transmitCW(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog);
    int background_rssi(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog);
    int current_rssi(Radio &radio);
    void sweep_transmitter(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog);
    int sweep_receiver(Packet &commandpacket, Radio &radio, ExternalWatchdog &watchdog);
    uint16_t query_radio_register(Packet &commandpacket, Radio &radio);
    float adjust_output_power(Packet &commandpacket, Radio &radio);
    //void toggle_frequency(Radio &radio);
    char background_S_level(Radio &radio);
};

#endif