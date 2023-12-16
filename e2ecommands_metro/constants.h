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
}

#endif