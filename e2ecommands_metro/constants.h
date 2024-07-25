#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

namespace constants
{
  extern const int pa_delay;  //settling time after enabling the PA before transmitting.  in microseconds.
  extern const unsigned long tx_delay; //tx_delay is the delay before switching from RX to TX in microseconds.  Once it switches, it sends a packet immediately.
  extern const byte clear_threshold; //clear_threshold is the threshold to declare the channel clear
  extern const byte callsign[7];
  extern const int bit_time; //used by beacon
  extern const int frequency;
  extern const int mtu_size;  //the MTU size defined in tncattach.  See main and Silversat Local Commands doc for more info.  Max size is 236 bytes.
  extern const uint8_t FEND;
  extern const uint8_t FESC;
  extern const uint8_t TFEND;
  extern const uint8_t TFESC;
  extern const float power;  //the power fraction
}

#endif