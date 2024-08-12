/**
 * @file beacon.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief beacon generator for Silversat
 * @version 1.0.1
 * @date 2022-11-08
 *
 * Beacon generation:
 
 * 4. This code has some placeholder code for the routine to create Morse code.
 *  Whatever new code is available should be relatively close to what's here.
 *  I've been told it exists, but no idea where it is.
 * 5. Once the beacon is done we need to reset the radio to the RX state.
 *
 */

#include "beacon.h"


// ************************************************************************/
/** sendbeacon - This function takes the command data sent by Avionics,
 * appends the satellite callsign, converts the command data to Morse code,
 * configures the radio in wire mode, transmits the beacon and returns the
 * radio board to normal operation in FULLRX mode.
 * takes seven variables:
 * beacon data, a four byte ASCII sequence (the fourth byte is added by the command processor)
 * config, an instance the ax_config structure, the modulation, the watchdog object, the efuse object,
 * and the radio object.
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
        dit(radio);
        dah(radio);
        break;

      case 'b':
        dah(radio);
        dit(radio);
        dit(radio);
        dit(radio);
        break;

      case 'c':
        dah(radio);
        dit(radio);
        dah(radio);
        dit(radio);
        break;

     case 'd':
        dah(radio);
        dit(radio);
        dit(radio);
        break;

      case 'e':
        dit(radio);
        break;
      
      case 'f':
        dit(radio);
        dit(radio);
        dah(radio);
        dit(radio);
        break;

      case 'g':
        dah(radio);
        dah(radio);
        dit(radio);
        break;

      case 'h':
        dit(radio);
        dit(radio);
        dit(radio);
        dit(radio);
        break;

      case 'i':
        dit(radio);
        dit(radio);
        break;

      case 'j':
        dit(radio);
        dah(radio);
        dah(radio);
        dah(radio);
        break;

      case 'k':
        dah(radio);
        dit(radio);
        dah(radio);
        break;

      case 'l':
        dit(radio);
        dah(radio);
        dit(radio);
        dit(radio);
        break;

      case 'm':
        dah(radio);
        dah(radio);
        break;
      
      case 'n':
        dah(radio); //morse(3);
        dit(radio); //morse();
        break;

      case 'o':
        dah(radio); //morse(3);
        dah(radio); //morse(3);
        dah(radio); //morse(3);
        break;

      case 'p':
        dit(radio); //morse();
        dah(radio); //morse(3);
        dah(radio); //morse(3);
        dit(radio); //morse();
        break;

      case 'q':
        dah(radio);
        dah(radio);
        dit(radio);
        dah(radio);
        break;

      case 'r':
        dit(radio); //morse();
        dah(radio); //morse(3);
        dit(radio); //morse();
        break;

      case 's':
        dit(radio); //morse();
        dit(radio); //morse();
        dit(radio); //morse();
        break;

      case 't':
        dah(radio); //morse(3);
        break;

      case 'u':
        dit(radio); //morse();
        dit(radio); //morse();
        dah(radio); //morse(3);
        break;

      case 'v':
        dit(radio); //morse();
        dit(radio); //morse();
        dit(radio); //morse();
        dah(radio); //morse(3);
        break;

      case 'w':
        dit(radio); //morse();
        dah(radio); //morse(3);
        dah(radio); //morse(3);
        break;

      case 'x':
        dah(radio); //morse(3);
        dit(radio); //morse();
        dit(radio); //morse();
        dah(radio); //morse(3);
        break;

      case 'y':
        dah(radio);
        dit(radio);
        dah(radio);
        dah(radio);
        break;

      case 'z':
        dah(radio); //morse(3);
        dah(radio); //morse(3);
        dit(radio); //morse();
        dit(radio); //morse();
        break;

      case '1':
        dit(radio); //morse();
        for (uint8_t i{0}; i < 4; i++)
            dah(radio); //morse(3);
        break;

      case '2':
        dit(radio); //morse();
        dit(radio); //morse();
        for (uint8_t i{0}; i < 3; i++)
            dah(radio); //morse(3);
        break;

      case '3':
        for (uint8_t i{0}; i < 3; i++)
            dit(radio); //morse();
        for (uint8_t i{0}; i < 2; i++)
            dah(radio); //morse(3);
        break;

      case '4':
        for (uint8_t i{0}; i < 4; i++)
            dit(radio); //morse();
        dah(radio); //morse(3);
        break;

      case '5':
        for (uint8_t i{0}; i < 5; i++)
            dit(radio); //morse();
        break;

      case '6':
        dah(radio); //morse(3);
        for (uint8_t i{0}; i < 4; i++)
            dit(radio); //morse();
        break;

      case '7':
        dah(radio); //morse(3);
        dah(radio); //morse(3);
        for (uint8_t i{0}; i < 3; i++)
            dit(radio); //morse();
        break;

      case '8':
        for (uint8_t i{0}; i < 3; i++)
            dah(radio); //morse(3);
        dit(radio); //morse();
        dit(radio); //morse();
        break;

      case '9':
        for (uint8_t i{0}; i < 4; i++)
            dah(radio); //morse(3);
        dit(radio); //morse();
        break;

      case '0':
        for (uint8_t i{0}; i < 5; i++)
            dah(radio); //morse(3);
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

/************************************************************************/
/** dah - sends a morse code "dah" using wire mode                    */
/************************************************************************/
void dah(Radio &radio)
{
    radio.key(3);
}

/************************************************************************/
/** dit() - sends a morse code "dit"                                    */
/************************************************************************/
void dit(Radio &radio)
{
    radio.key(1);
}