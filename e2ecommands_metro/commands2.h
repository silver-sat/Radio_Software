/**
 * @file commands2.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2024-07-19

 */

#define DEBUG

#ifndef COMMANDS2_H
#define COMMANDS2_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512 // 4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 // 32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include <SPI.h>
#include <Wire.h>
#include <LibPrintf.h>
#include <CircularBuffer.h>
#include <cstdlib> //for atoi function, may replace this with String functions, but it's working...
#include <Arduino.h>

#include "beacon.h"
#include "KISS.h"
#include "constants.h"
#include "ax.h"
#include <Temperature_LM75_Derived.h>
#include "ExternalWatchdog.h"
#include "efuse.h"
#include "radiohw.h"
#include "antenna.h"

struct packet 
{
    byte commandcode;
    unsigned char commandbody[30];  //looks like the longest command is 26 bytes
};

class Command
{
public:
    void processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, packet &commandpacket); // determines if packet is destined for other end of link, and if not, extracts the body
    void processcommand(CircularBuffer<byte, DATABUFFSIZE> &databuffer, packet &commandpacket, ax_config &config, ax_modulation &modulation, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio); // calls the appropriate command based on the command code

private:
    String response;

    // command responses
    void sendACK(byte code);
    void sendNACK(byte code);
    void sendResponse(byte code, String &response);

    // commands
    void beacon(packet &commandpacket, ax_config &config, ax_modulation &modulation, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio);
    void manual_antenna_release(packet &commandpacket, ExternalWatchdog &watchdog, String &response);
    void status(packet &commandpacket, ax_config &config, ax_modulation &modulation, Efuse &efuse, Radio &radio, String &response);
    void halt(CircularBuffer<byte, DATABUFFSIZE> &databuffer, ax_modulation &modulation, packet &commandpacket, ax_config &config, Radio &radio);
    int modify_frequency(packet &commandpacket, ax_config &config);
    void modify_mode(packet &commandpacket, ax_config &config, ax_modulation &modulation);
    void doppler_frequencies(packet &commandpacket, ax_config &config, ax_modulation &modulation);
    void transmit_callsign(CircularBuffer<byte, DATABUFFSIZE> &databuffer);
    //reset_5V() is handled in the efuse class
    void transmitCW(packet &commandpacket, ax_config &config, ax_modulation &modulation, Radio &radio, ExternalWatchdog &watchdog);
    int background_rssi(packet &commandpacket, ax_config &config, ax_modulation &modulation, Radio &radio, ExternalWatchdog &watchdog);
    int current_rssi(packet &commandpacket, ax_config &config);
    void sweep_transmitter(packet &commandpacket, ax_config &config, ax_modulation &modulation, Radio &radio, ExternalWatchdog &watchdog);
    void sweep_receiver();
    uint16_t query_radio_register(packet &commandpacket, ax_config &config);
    float adjust_output_power(packet &commandpacket, ax_config &config, ax_modulation &modulation);
    void toggle_frequency(ax_config &config, ax_modulation &modulation, Radio &radio);
};

#endif