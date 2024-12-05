/**
 * @file constants.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief constants used with Silversat
 * @version 1.0.1
 * @date 2024-08-12
 *
 * These are all the constants used in the radio code
 * pa_delay = settling time after enabling the PA before transmitting.  in microseconds.
 * tx_delay = the delay before switching from RX to TX in microseconds.  Once it switches, it sends a packet immediately.
 * clear_theshold = the threshold to declare the channel clear
 * callsign = the silversat group callsign
 * bit_time = the ASK bit time used to define a 1 or a 0.  Think of it more as a sample rate.
 * frequency = the default Silversat radio frequency.  Used at startup for both transmit and receive.
 * mtu_size = the mtu size defined in tncattach.  This defines the packet size.  See main and Silversat Local Commands doc for more info.
 *  Max packet size is 236 bytes (including reed solomon bytes) (one FIFO chunk (240 bytes), less FIFO headers)
 * FEND = KISS FEND byte
 * FESC = KISS FESC byte
 * TFEND = KISS TFEND byte
 * TFESC = KISS TFESC byte
 * power = the power fraction expressed as a percentage of maximum power.  remember that the max power for single ended is half of the power for differential.
 */

#include "constants.h"

namespace constants
{
    extern const int pa_delay{100};
    extern const unsigned long tx_delay{200000}; // 200000 fixed collisions...
    extern const byte clear_threshold{0xB4}; //B4 = 180, somewhere around -88dBm.  Should be more like 156.
    extern const byte callsign[7]{"WP2XGW"};  //KC3VVW
    extern const int bit_time{59};
    extern const int frequency{437175000};
    extern const int mtu_size{200};  //not actually used
    extern const int max_packet_size{255};
    extern const byte FEND{0xC0};
    extern const byte FESC{0xDB};
    extern const byte TFEND{0xDC};
    extern const byte TFESC{0xDD};
    extern const float power{0.5};
    extern const uint32_t max_delta_carrier{3000};
    extern const byte preamble_length{16};
    extern const String version{"1.12_C"};
}