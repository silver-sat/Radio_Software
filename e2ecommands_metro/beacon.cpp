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
void sendbeacon(byte beacondata[], int beaconstringlength, ax_config& config, ax_modulation& modulation, ExternalWatchdog& watchdog, Efuse& efuse) {
  //set the tx path..do this before loading the parameters
  //ax_set_tx_path(&config, AX_TRANSMIT_PATH_SE);   // not needed, if system set for SE, then it always uses right path for Tx/Rx
  ax_off(&config);
  ax_init(&config);  //this does a reset, so needs to be first

  //load the RF parameters
  ax_default_params(&config, &ask_modulation);

  pinfunc_t func = 0x84;  // put the radio in wire mode
  ax_set_pinfunc_data(&config, func);  //remember to set this back when done!

  debug_printf("config variable values: \n");
  debug_printf("tcxo frequency: %u \n", uint(config.f_xtal));
  debug_printf("synthesizer A frequency: %u \n", uint(config.synthesiser.A.frequency));
  debug_printf("synthesizer B frequency: %u \n", uint(config.synthesiser.B.frequency));
  debug_printf("status: %x \n", ax_hw_status());

  //set the RF switch to transmit
  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);
  //digitalWrite(AX5043_DATA, HIGH);

  ax_tx_on(&config, &ask_modulation);
  //digitalWrite(GPIO15, HIGH);  //indicate that a beacon is happening

  //AX5043 is in wire mode and setup for ASK with single ended transmit path

  for (int i=0; i < beaconstringlength; i++) //size of callsign includes null term, so we have to subtract one and then add the 4 bytes to get 3
  {
    debug_printf("current character %c \n", beacondata[i]);
    switch (tolower(beacondata[i]))
    {
      case 'a':
        dit();
        dah();
        break;

      case 'b':
        dah();
        dit();
        dit();
        dit();
        break;

      case 'c':
        dah();
        dit();
        dah();
        dit();
        break;

     case 'd':
        dah();
        dit();
        dit();
        break;

      case 'e':
        dit();
        break;
      
      case 'f':
        dit();
        dit();
        dah();
        dit();
        break;

      case 'g':
        dah();
        dah();
        dit();
        break;

      case 'h':
        dit();
        dit();
        dit();
        dit();
        break;

      case 'i':
        dit();
        dit();
        break;

      case 'j':
        dit();
        dah();
        dah();
        dah();
        break;

      case 'k':
        dah();
        dit();
        dah();
        break;

      case 'l':
        dit();
        dah();
        dit();
        dit();
        break;

      case 'm':
        dah();
        dah();
        break;
      
      case 'n':
        dah(); //morse(3);
        dit(); //morse();
        break;

      case 'o':
        dah(); //morse(3);
        dah(); //morse(3);
        dah(); //morse(3);
        break;

      case 'p':
        dit(); //morse();
        dah(); //morse(3);
        dah(); //morse(3);
        dit(); //morse();
        break;

      case 'q':
        dah();
        dah();
        dit();
        dah();
        break;

      case 'r':
        dit(); //morse();
        dah(); //morse(3);
        dit(); //morse();
        break;

      case 's':
        dit(); //morse();
        dit(); //morse();
        dit(); //morse();
        break;

      case 't':
        dah(); //morse(3);
        break;

      case 'u':
        dit(); //morse();
        dit(); //morse();
        dah(); //morse(3);
        break;

      case 'v':
        dit(); //morse();
        dit(); //morse();
        dit(); //morse();
        dah(); //morse(3);
        break;

      case 'w':
        dit(); //morse();
        dah(); //morse(3);
        dah(); //morse(3);
        break;

      case 'x':
        dah(); //morse(3);
        dit(); //morse();
        dit(); //morse();
        dah(); //morse(3);
        break;

      case 'y':
        dah();
        dit();
        dah();
        dah();
        break;

      case 'z':
        dah(); //morse(3);
        dah(); //morse(3);
        dit(); //morse();
        dit(); //morse();
        break;

      case '1':
        dit(); //morse();
        for (uint8_t i{0}; i < 4; i++)
            dah(); //morse(3);
        break;

      case '2':
        dit(); //morse();
        dit(); //morse();
        for (uint8_t i{0}; i < 3; i++)
            dah(); //morse(3);
        break;

      case '3':
        for (uint8_t i{0}; i < 3; i++)
            dit(); //morse();
        for (uint8_t i{0}; i < 2; i++)
            dah(); //morse(3);
        break;

      case '4':
        for (uint8_t i{0}; i < 4; i++)
            dit(); //morse();
        dah(); //morse(3);
        break;

      case '5':
        for (uint8_t i{0}; i < 5; i++)
            dit(); //morse();
        break;

      case '6':
        dah(); //morse(3);
        for (uint8_t i{0}; i < 4; i++)
            dit(); //morse();
        break;

      case '7':
        dah(); //morse(3);
        dah(); //morse(3);
        for (uint8_t i{0}; i < 3; i++)
            dit(); //morse();
        break;

      case '8':
        for (uint8_t i{0}; i < 3; i++)
            dah(); //morse(3);
        dit(); //morse();
        dit(); //morse();
        break;

      case '9':
        for (uint8_t i{0}; i < 4; i++)
            dah(); //morse(3);
        dit(); //morse();
        break;

      case '0':
        for (uint8_t i{0}; i < 5; i++)
            dah(); //morse(3);
        break;
      
      case 0x20:  //space
        delay(3*constants::bit_time);
        break;

      default:
        debug_printf("not sending \n");
    }   
    delay(3*constants::bit_time);
    watchdog.trigger();
    if (efuse.overcurrent(true)) //here we're only checking if it exceeds the upper limit.
    {
        // create an reset packet and put it in the CMD TX queue
        debug_printf("Overcurrent!! \r\n");      // uh oh!
        byte resetpacket[] = {0xC0, 0x0F, 0xC0}; // generic form of nack packet
        Serial0.write(resetpacket, 3);
    }
  }

  debug_printf("status: %x \n", ax_hw_status());
  //just to be sure, turn the PA off
  digitalWrite(PAENABLE, LOW);
  digitalWrite(PIN_LED_TX, LOW);

  //and set the switch controls to the receive path.
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);

  func = 2;

  //drop out of wire mode
  ax_set_pinfunc_data(&config, func);

  //now put it back...we need to be in receive mode...should we check that avionics might be trying to send a beacon when it's not supposed to?

  ax_off(&config);  //turn the radio off
  //debug_printf("radio off \n");
  ax_init(&config);  //this does a reset, so probably needs to be first, this hopefully takes us out of wire mode too
  debug_printf("radio init \n");
  //load the RF parameters
  ax_default_params(&config, &modulation);  //ax_modes.c for RF parameters
  
  debug_printf("default params loaded \n");
  ax_rx_on(&config, &modulation);

  debug_printf("receiver on \n");
  debug_printf("status: %x \n", ax_hw_status());
  debug_printf("i'm done and back to receive \n");
  //digitalWrite(GPIO15, LOW);  //indicate that a beacon is done
}

/************************************************************************/
/** dah - sends a morse code "dah" using wire mode                    */
/************************************************************************/
void dah()
{
  digitalWrite(PAENABLE, HIGH);
  //delay(PAdelay); //let the pa bias stabilize
  digitalWrite(PIN_LED_TX, HIGH);
  digitalWrite(AX5043_DATA, HIGH);

  delay(3*constants::bit_time);

  digitalWrite(AX5043_DATA, LOW);
  digitalWrite(PAENABLE, LOW); //turn off the PA
  digitalWrite(PIN_LED_TX, LOW);

  delay(constants::bit_time);
}

/************************************************************************/
/** dit() - sends a morse code "dit"                                    */
/************************************************************************/
void dit()
{
  digitalWrite(PAENABLE, HIGH);
  //delay(PAdelay); //let the pa bias stabilize
  digitalWrite(PIN_LED_TX, HIGH);
  digitalWrite(AX5043_DATA, HIGH);

  delay(constants::bit_time);

  digitalWrite(AX5043_DATA, LOW);
  digitalWrite(PAENABLE, LOW); //turn off the PA
  digitalWrite(PIN_LED_TX, LOW);

  delay(constants::bit_time);
}

