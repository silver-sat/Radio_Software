

#include "constants.h"

namespace constants
{
  extern const int pa_delay {100};
  extern const unsigned long tx_delay {2000};
  extern const byte clear_threshold {0xA0};
  extern const byte callsign[7] {"KC3VVW"};
  extern const int bit_time {100};
  extern const int frequency {433000000};
  extern const int mtu_size {200};
  extern const uint8_t FEND {0xC0};
  extern const uint8_t FESC {0xDB};
  extern const uint8_t TFEND {0xDC};
  extern const uint8_t TFESC {0xDD};
  extern const float power {1.0};  
}