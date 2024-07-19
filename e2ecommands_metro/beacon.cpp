/**
 * @file beacon.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief beacon generator for Silversat
 * @version 1.0.1
 * @date 2022-11-08
 *
 * Beacon generation:
 * 1.  Switch the AX5043 into WIRE mode using ASK modulation.  When in WIRE mode
 *  the data rate can instead be thought of as sampling rate.  So, if you have
 *  a data rate of 100 bps, then the smallest time sample is 10 mSeconds.
 *  Since you're in wire mode, you're clocking out a 1 if data is high, every
 *  1/datarate seconds.
 * 2. To enter wire mode write 0x84 to PINFUNCDATA (there's a library call for
 *  that).
 * 3.  THIS IS IMPORTANT: The PA will go kablooey if you turn on the RF signal
 *  before the power is applied.  I had three (at ~$20 each) PA's blow up this
 *  way.  Make sure the output switch is set to the load (TX/~RX high, ~TX/RX
 *  low), and that a load is attached.  You must also switch the TX path to
 *  single ended.  Only then can you safely turn on the RF signal from the
 *  AX5043.
 * 4. This code has some placeholder code for the routine to create Morse code.
 *  Whatever new code is available should be relatively close to what's here.
 *  I've been told it exists, but no idea where it is.
 * 5. Once the beacon is done we need to reset the radio to the RX state.
 *
 */

#include "beacon.h"

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

// ************************************************************************/
/** sendbeacon - This function takes the command data sent by Avionics,
 * appends the satellite callsign, converts the command data to Morse code,
 * configures the radio in wire mode, transmits the beacon and returns the
 * radio board to normal operation in FULLRX mode.
 * takes two variables:
 * command data, a four byte ASCII sequence
 * config, an instance the ax_config structure
// ************************************************************************/
void sendbeacon(byte beacondata[], int beaconstringlength, ax_config& config, ax_modulation& modulation, ExternalWatchdog& watchdog, Efuse& efuse, Radio& radio) {
  
  radio.beaconMode(config, ask_modulation);
  
  // AX5043 is in wire mode and setup for ASK with single ended transmit path

  for (int i=0; i < beaconstringlength; i++) //size of callsign includes null term, so we have to subtract one and then add the 4 bytes to get 3
  {
    debug_printf("current character %c \r\n", beacondata[i]);
    switch (tolower(beacondata[i]))
    {
      case 'a':
        radio.dit();
        radio.dah();
        break;

      case 'b':
        radio.dah();
        radio.dit();
        radio.dit();
        radio.dit();
        break;

      case 'c':
        radio.dah();
        radio.dit();
        radio.dah();
        radio.dit();
        break;

     case 'd':
        radio.dah();
        radio.dit();
        radio.dit();
        break;

      case 'e':
        radio.dit();
        break;
      
      case 'f':
        radio.dit();
        radio.dit();
        radio.dah();
        radio.dit();
        break;

      case 'g':
        radio.dah();
        radio.dah();
        radio.dit();
        break;

      case 'h':
        radio.dit();
        radio.dit();
        radio.dit();
        radio.dit();
        break;

      case 'i':
        radio.dit();
        radio.dit();
        break;

      case 'j':
        radio.dit();
        radio.dah();
        radio.dah();
        radio.dah();
        break;

      case 'k':
        radio.dah();
        radio.dit();
        radio.dah();
        break;

      case 'l':
        radio.dit();
        radio.dah();
        radio.dit();
        radio.dit();
        break;

      case 'm':
        radio.dah();
        radio.dah();
        break;
      
      case 'n':
        radio.dah(); //morse(3);
        radio.dit(); //morse();
        break;

      case 'o':
        radio.dah(); //morse(3);
        radio.dah(); //morse(3);
        radio.dah(); //morse(3);
        break;

      case 'p':
        radio.dit(); //morse();
        radio.dah(); //morse(3);
        radio.dah(); //morse(3);
        radio.dit(); //morse();
        break;

      case 'q':
        radio.dah();
        radio.dah();
        radio.dit();
        radio.dah();
        break;

      case 'r':
        radio.dit(); //morse();
        radio.dah(); //morse(3);
        radio.dit(); //morse();
        break;

      case 's':
        radio.dit(); //morse();
        radio.dit(); //morse();
        radio.dit(); //morse();
        break;

      case 't':
        radio.dah(); //morse(3);
        break;

      case 'u':
        radio.dit(); //morse();
        radio.dit(); //morse();
        radio.dah(); //morse(3);
        break;

      case 'v':
        radio.dit(); //morse();
        radio.dit(); //morse();
        radio.dit(); //morse();
        radio.dah(); //morse(3);
        break;

      case 'w':
        radio.dit(); //morse();
        radio.dah(); //morse(3);
        radio.dah(); //morse(3);
        break;

      case 'x':
        radio.dah(); //morse(3);
        radio.dit(); //morse();
        radio.dit(); //morse();
        radio.dah(); //morse(3);
        break;

      case 'y':
        radio.dah();
        radio.dit();
        radio.dah();
        radio.dah();
        break;

      case 'z':
        radio.dah(); //morse(3);
        radio.dah(); //morse(3);
        radio.dit(); //morse();
        radio.dit(); //morse();
        break;

      case '1':
        radio.dit(); //morse();
        for (uint8_t i{0}; i < 4; i++)
            radio.dah(); //morse(3);
        break;

      case '2':
        radio.dit(); //morse();
        radio.dit(); //morse();
        for (uint8_t i{0}; i < 3; i++)
            radio.dah(); //morse(3);
        break;

      case '3':
        for (uint8_t i{0}; i < 3; i++)
            radio.dit(); //morse();
        for (uint8_t i{0}; i < 2; i++)
            radio.dah(); //morse(3);
        break;

      case '4':
        for (uint8_t i{0}; i < 4; i++)
            radio.dit(); //morse();
        radio.dah(); //morse(3);
        break;

      case '5':
        for (uint8_t i{0}; i < 5; i++)
            radio.dit(); //morse();
        break;

      case '6':
        radio.dah(); //morse(3);
        for (uint8_t i{0}; i < 4; i++)
            radio.dit(); //morse();
        break;

      case '7':
        radio.dah(); //morse(3);
        radio.dah(); //morse(3);
        for (uint8_t i{0}; i < 3; i++)
            radio.dit(); //morse();
        break;

      case '8':
        for (uint8_t i{0}; i < 3; i++)
            radio.dah(); //morse(3);
        radio.dit(); //morse();
        radio.dit(); //morse();
        break;

      case '9':
        for (uint8_t i{0}; i < 4; i++)
            radio.dah(); //morse(3);
        radio.dit(); //morse();
        break;

      case '0':
        for (uint8_t i{0}; i < 5; i++)
            radio.dah(); //morse(3);
        break;
      
      case 0x20:  //space
        delay(3*constants::bit_time);
        break;

      default:
        debug_printf("not sending \r\n");
    }   
    delay(3*constants::bit_time);
    watchdog.trigger();
    efuse.overcurrent(true); //here we're only checking if it exceeds the upper limit.
    
  }

  radio.dataMode(config, modulation);
  debug_printf("status: %x \r\n", ax_hw_status());
}