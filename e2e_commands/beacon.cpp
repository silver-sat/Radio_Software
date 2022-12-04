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

void sendbeacon(unsigned char& commanddata, ax_config& config) {
  //for now let's just print that out, because it needs to be converted to morse code, a la the format above

  char beaconarray[24] = CALLSIGN;
  strcat(beaconarray, (char*) commanddata); //now we have the beacon string
  int beaconlen = strlen((char*)beaconarray);
  //debug_printf("the beacon string is %s \n", beaconarray);

  //Serial2.write(beaconarray, beaconlen);  //for the moment...this will go to the radio.

  //set the tx path..do this before loading the parameters
  ax_set_tx_path(&config, AX_TRANSMIT_PATH_SE);
   
  ax_init(&config);  //this does a reset, so probably needs to be first
  
  //load the RF parameters
  ax_default_params(&config, &ask_modulation);

  pinfunc_t func = 0x84;
  ax_set_pinfunc_data(&config, func);  //remember to set this back when done!

  //set the RF switch to transmit
  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);

  ax_tx_on(&config, &ask_modulation);

  //AX5043 is now in CW mode and the output path is set for single ended
  
  for (int i=0;i<beaconlen-1;i++)  //don't include null terminator
  {
    debug_printf("current character %c \n", char(beaconarray[i]));
    switch (beaconarray[i])
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

  //and set the switch controls to the receive path.  
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);
  
  //drop out of wire mode
  ax_set_pinfunc_data(&config, func);

  
  //now put it back...we need to be in receive mode...should we check that avionics might be trying to send a beacon when it's not supposed to?
  //set the tx path..do this before loading the parameters
  ax_set_tx_path(&config, AX_TRANSMIT_PATH_DIFF);
  
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

void dash()
{
  digitalWrite(PAENABLE, HIGH);
  //delay(PAdelay); //let the pa bias stabilize
  digitalWrite(PIN_LED_TXL, HIGH);
  digitalWrite(AX5043_DATA, HIGH);
    
  delay(3*BITTIME);

  digitalWrite(AX5043_DATA, LOW);
  digitalWrite(PAENABLE, LOW); //turn off the PA
  digitalWrite(PIN_LED_TXL, LOW);
  
  delay(BITTIME);
}

void dot()
{
  digitalWrite(PAENABLE, HIGH);
  //delay(PAdelay); //let the pa bias stabilize
  digitalWrite(PIN_LED_TXL, HIGH);
  digitalWrite(AX5043_DATA, HIGH);
    
  delay(BITTIME);

  digitalWrite(AX5043_DATA, LOW);
  digitalWrite(PAENABLE, LOW); //turn off the PA
  digitalWrite(PIN_LED_TXL, LOW);
  
  delay(BITTIME);
}
