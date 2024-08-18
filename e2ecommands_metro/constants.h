/**
 * @file constants.h
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
 * max_delta_carrier = range for the AFC loop, usually based on oscillator tolerance, in kHz
 * preamble_length = number of preamble bytes to send
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

namespace constants
{
  extern const int pa_delay;  
  extern const unsigned long tx_delay; 
  extern const byte clear_threshold; 
  extern const byte callsign[7];
  extern const int bit_time;
  extern const int frequency;
  extern const int mtu_size;
  extern const uint8_t FEND;
  extern const uint8_t FESC;
  extern const uint8_t TFEND;
  extern const uint8_t TFESC;
  extern const float power;
  extern const uint32_t max_delta_carrier;
  extern const uint8_t preamble_length;
}

#endif