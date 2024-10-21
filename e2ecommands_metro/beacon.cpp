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
 * takes five variables:
 * beacon data, a four byte ASCII sequence (the fourth byte is added by the command processor)
 * beaconstringlength, the length of the beacon string
 * the watchdog object, the efuse object,
 * and the radio object.
// ************************************************************************/
void sendbeacon(byte beacondata[], int beaconstringlength, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio)
{

    radio.beaconMode();
    // AX5043 is in wire mode and setup for ASK with single ended transmit path

    for (int i = 0; i < beaconstringlength; i++) // size of callsign includes null term, so we have to subtract one and then add the 4 bytes to get 3
    {
        Log.notice(F("current character %c\r\n"), beacondata[i]);
        switch (tolower(beacondata[i]))
        {
        case 'a':
            dit(radio, efuse);
            dah(radio, efuse);
            break;

        case 'b':
            dah(radio, efuse);
            dit(radio, efuse);
            dit(radio, efuse);
            dit(radio, efuse);
            break;

        case 'c':
            dah(radio, efuse);
            dit(radio, efuse);
            dah(radio, efuse);
            dit(radio, efuse);
            break;

        case 'd':
            dah(radio, efuse);
            dit(radio, efuse);
            dit(radio, efuse);
            break;

        case 'e':
            dit(radio, efuse);
            break;

        case 'f':
            dit(radio, efuse);
            dit(radio, efuse);
            dah(radio, efuse);
            dit(radio, efuse);
            break;

        case 'g':
            dah(radio, efuse);
            dah(radio, efuse);
            dit(radio, efuse);
            break;

        case 'h':
            dit(radio, efuse);
            dit(radio, efuse);
            dit(radio, efuse);
            dit(radio, efuse);
            break;

        case 'i':
            dit(radio, efuse);
            dit(radio, efuse);
            break;

        case 'j':
            dit(radio, efuse);
            dah(radio, efuse);
            dah(radio, efuse);
            dah(radio, efuse);
            break;

        case 'k':
            dah(radio, efuse);
            dit(radio, efuse);
            dah(radio, efuse);
            break;

        case 'l':
            dit(radio, efuse);
            dah(radio, efuse);
            dit(radio, efuse);
            dit(radio, efuse);
            break;

        case 'm':
            dah(radio, efuse);
            dah(radio, efuse);
            break;

        case 'n':
            dah(radio, efuse); // morse(3);
            dit(radio, efuse); // morse();
            break;

        case 'o':
            dah(radio, efuse); // morse(3);
            dah(radio, efuse); // morse(3);
            dah(radio, efuse); // morse(3);
            break;

        case 'p':
            dit(radio, efuse); // morse();
            dah(radio, efuse); // morse(3);
            dah(radio, efuse); // morse(3);
            dit(radio, efuse); // morse();
            break;

        case 'q':
            dah(radio, efuse);
            dah(radio, efuse);
            dit(radio, efuse);
            dah(radio, efuse);
            break;

        case 'r':
            dit(radio, efuse); // morse();
            dah(radio, efuse); // morse(3);
            dit(radio, efuse); // morse();
            break;

        case 's':
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            break;

        case 't':
            dah(radio, efuse); // morse(3);
            break;

        case 'u':
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            dah(radio, efuse); // morse(3);
            break;

        case 'v':
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            dah(radio, efuse); // morse(3);
            break;

        case 'w':
            dit(radio, efuse); // morse();
            dah(radio, efuse); // morse(3);
            dah(radio, efuse); // morse(3);
            break;

        case 'x':
            dah(radio, efuse); // morse(3);
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            dah(radio, efuse); // morse(3);
            break;

        case 'y':
            dah(radio, efuse);
            dit(radio, efuse);
            dah(radio, efuse);
            dah(radio, efuse);
            break;

        case 'z':
            dah(radio, efuse); // morse(3);
            dah(radio, efuse); // morse(3);
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            break;

        case '1':
            dit(radio, efuse); // morse();
            for (uint8_t i{0}; i < 4; i++)
                dah(radio, efuse); // morse(3);
            break;

        case '2':
            dit(radio, efuse); // morse();
            dit(radio, efuse); // morse();
            for (uint8_t i{0}; i < 3; i++)
                dah(radio, efuse); // morse(3);
            break;

        case '3':
            for (uint8_t i{0}; i < 3; i++)
                dit(radio, efuse); // morse();
            for (uint8_t i{0}; i < 2; i++)
                dah(radio, efuse); // morse(3);
            break;

        case '4':
            for (uint8_t i{0}; i < 4; i++)
                dit(radio, efuse); // morse();
            dah(radio, efuse);     // morse(3);
            break;

        case '5':
            for (uint8_t i{0}; i < 5; i++)
                dit(radio, efuse); // morse();
            break;

        case '6':
            dah(radio, efuse); // morse(3);
            for (uint8_t i{0}; i < 4; i++)
                dit(radio, efuse); // morse();
            break;

        case '7':
            dah(radio, efuse); // morse(3);
            dah(radio, efuse); // morse(3);
            for (uint8_t i{0}; i < 3; i++)
                dit(radio, efuse); // morse();
            break;

        case '8':
            for (uint8_t i{0}; i < 3; i++)
                dah(radio, efuse); // morse(3);
            dit(radio, efuse);     // morse();
            dit(radio, efuse);     // morse();
            break;

        case '9':
            for (uint8_t i{0}; i < 4; i++)
                dah(radio, efuse); // morse(3);
            dit(radio, efuse);     // morse();
            break;

        case '0':
            for (uint8_t i{0}; i < 5; i++)
                dah(radio, efuse); // morse(3);
            break;

        case 0x20: // space
            delay(3 * constants::bit_time);
            break;

        default:
            Log.error("not sending\r\n");
        }
        delay(3 * constants::bit_time);
        watchdog.trigger();
        efuse.overcurrent(true); // here we're only checking if it exceeds the upper limit.
    }
    radio.dataMode();
    Log.trace(F("status: %X\r\n"), ax_hw_status());  //allowed for debug
}

/************************************************************************/
/** dah - sends a morse code "dah" using wire mode                    */
/************************************************************************/
void dah(Radio &radio, Efuse &efuse)
{
    radio.key(3, efuse);
}

/************************************************************************/
/** dit() - sends a morse code "dit"                                    */
/************************************************************************/
void dit(Radio &radio, Efuse &efuse)
{
    radio.key(1, efuse);
}