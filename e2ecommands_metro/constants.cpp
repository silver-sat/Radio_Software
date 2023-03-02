

#include "constants.h"

namespace constants
{
  extern const int pa_delay {100};
  extern const unsigned long tx_delay {2000};
  extern const byte clear_threshold {0xA0};
  extern const byte callsign[7] {"MYCALL"}; //allowing for a 7 character call sign
  extern const int bit_time {200};
  extern const int frequency {433000000};
  extern const int mtu_size {200};
}