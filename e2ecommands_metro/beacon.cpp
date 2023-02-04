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
 * 5. Once the beacon is done we need to re-setup the back to the RX state.
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
void sendbeacon(unsigned char& beacondata, ax_config& config) {
  //for now let's just print that out, because it needs to be converted to morse code, a la the format above
  unsigned char beaconstring[12]; //beaconstring consists of callsign (7 bytes) and four beacon characters (4 bytes) + plus terminator (1 byte)
  unsigned char callsign[] {CALLSIGN};
  
  memcpy(beaconstring, callsign, sizeof(callsign));
  memcpy(&beaconstring[sizeof(callsign)], &beacondata, sizeof(beacondata));

  //set the tx path..do this before loading the parameters
  //ax_set_tx_path(&config, AX_TRANSMIT_PATH_SE);   // not needed, if system set for SE, then it always uses right path for Tx/Rx

  ax_init(&config);  //this does a reset, so probably needs to be first

  //load the RF parameters
  ax_default_params(&config, &ask_modulation);

  pinfunc_t func = 0x84;
  ax_set_pinfunc_data(&config, func);  //remember to set this back when done!

  //set the RF switch to transmit
  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);

  ax_tx_on(&config, &ask_modulation);

  //AX5043 is in wire mode and setup for ASK with single ended transmit path

  for (int i=0; i < (int)sizeof(beaconstring); i++)
  {
    debug_printf("current character %c \n", beaconarray[i]);
    switch (beaconstring[i])
    {
      case 'a':
      case 'A':
        dot();
        dash();
        break;

      case 'b':
      case 'B':
        dash();
        dot();
        dot();
        dot();
        break;

      case 'c':
      case 'C':
        dash();
        dot();
        dash();
        dot();
        break;

     case 'd':
     case 'D':
        dash();
        dot();
        dot();
        break;

      case 'l':
      case 'L':
        dot();
        dash();
        dot();
        dot();
        break;

      case 'm':
      case 'M':
        dash();
        dash();
        break;

      case 'q':
      case 'Q':
        dash();
        dash();
        dot();
        dash();
        break;

      case 'y':
      case 'Y':
        dash();
        dot();
        dash();
        dash();
        break;
    }
  }

  debug_printf("status: %x \n", ax_hw_status());
  //just to be sure, turn the PA off
  digitalWrite(PAENABLE, LOW);
  digitalWrite(PIN_LED_TX, LOW);

  //and set the switch controls to the receive path.
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);

  func = 1;

  //drop out of wire mode
  ax_set_pinfunc_data(&config, func);

  //now put it back...we need to be in receive mode...should we check that avionics might be trying to send a beacon when it's not supposed to?
  //set the tx path..do this before loading the parameters
  //ax_set_tx_path(&config, AX_TRANSMIT_PATH_DIFF);  //this is unnecessary

  //ax_off(&config);  //turn the radio off
  //debug_printf("radio off \n");
  ax_init(&config);  //this does a reset, so probably needs to be first, this hopefully takes us out of wire mode too
  debug_printf("radio init \n");
  //load the RF parameters
  ax_default_params(&config, &fsk_modulation);  //ax_modes.c for RF parameters
  debug_printf("transmit path %s \n", config.transmit_path);
  debug_printf("default params loaded \n");
  ax_rx_on(&config, &fsk_modulation);
  debug_printf("receiver on \n");

  debug_printf("status: %x \n", ax_hw_status());

  debug_printf("i'm done and back to receive \n");
}

/************************************************************************/
/** dash - sends a morse code "dash" using wire mode                    */
/************************************************************************/
void dash()
{
  digitalWrite(PAENABLE, HIGH);
  //delay(PAdelay); //let the pa bias stabilize
  digitalWrite(PIN_LED_TX, HIGH);
  digitalWrite(AX5043_DATA, HIGH);

  delay(3*BITTIME);

  digitalWrite(AX5043_DATA, LOW);
  digitalWrite(PAENABLE, LOW); //turn off the PA
  digitalWrite(PIN_LED_TX, LOW);

  delay(BITTIME);
}

/************************************************************************/
/** dot() - sends a morse code "dot"                                    */
/************************************************************************/
void dot()
{
  digitalWrite(PAENABLE, HIGH);
  //delay(PAdelay); //let the pa bias stabilize
  digitalWrite(PIN_LED_TX, HIGH);
  digitalWrite(AX5043_DATA, HIGH);

  delay(BITTIME);

  digitalWrite(AX5043_DATA, LOW);
  digitalWrite(PAENABLE, LOW); //turn off the PA
  digitalWrite(PIN_LED_TX, LOW);

  delay(BITTIME);
}

