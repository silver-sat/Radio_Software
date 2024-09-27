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
    extern const unsigned long tx_delay{1000}; //was 2000, trying to see if I can fix collisions...
    extern const byte clear_threshold{0x98};
    extern const byte callsign[7]{"KC3VVW"};
    extern const int bit_time{59};
    extern const int frequency{433000000};
    extern const int mtu_size{200};
    extern const uint8_t FEND{0xC0};
    extern const uint8_t FESC{0xDB};
    extern const uint8_t TFEND{0xDC};
    extern const uint8_t TFESC{0xDD};
    extern const float power{0.5};
    extern const uint32_t max_delta_carrier{3000};
    extern const uint8_t preamble_length{18};
    extern const String version{"1.5"};
}