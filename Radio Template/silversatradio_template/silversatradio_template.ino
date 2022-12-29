/**
 * @file silversatradio_template.ino
 * @author Tom Conrad (tom@silversat.org)
 * @brief template for talking to the AX5043
 * @version 1.0.1
 * @date 2022-12-23
 *
 * What we're testing:
 * 1. Proper setup and configuration of the AX library
 *
 *
 * Serial 2 is no longer needed.  Beacons will be issued by changing the radio state to wire mode using ASK
 * Serial 0 represents the Ground station or Avionics.  Commands and remote Command responses are sent via Serial0 (as data), and local responses are issued to Serial0 by the radio board.
 * Serial 1 is the data port.  The intent is for this to almost seem like a packet pass-through. (essentially one of the tests)
 * Serial is the USB debug port
 *
 * 2 RS422/RS485 converters are required to run the test PER SYSTEM, but 2 total will do at first since we're not really trying to pass data just yet (although it should work).
 * There is also a *very* rudimentary python script that issues commands (canned) and listens for responses.
 * Unless you're working with a Metro, in which case you need 2 USB to TTL Serial (3.3V).
 * 
 *
 * ALSO NOTE: you must use HDLC to use FEC
 */

//#define DEBUG

//this define is required by AX library.  It tells the radio which path to use for Transmit.  Receive is always differential.  
//Use DIFF for the Metro/Eval boards, SE for the Silversat boards
#define _AX_TX_DIFF
//#define _AX_TX_SE

//this allows us to turn on/off debug messages and gives us printf formatting using the LibPrintf library.
#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#include <LibPrintf.h>

//interface to the radio
#include <SPI.h>

//interface to the PA temp sensor
#include <Wire.h>
#include <Temperature_LM75_Derived.h>

//the AX library files
#include "ax.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"

// delay values for time to wait after setting T/R lines and turning on PA, and time to allow PA to stabilize.
//This is a guess at the moment and probably way too large.
#define PAdelay 100
#define TXDELAY  2000  //delay before switching from RX to TX.  Once it switches, it sends a packet immediately.  
#define CLEARTHRESH 0xA0 //this is the threshold to declare the channel clear

//radio config and interface
ax_packet rx_pkt;  //instance of packet structure

//radio config
ax_config config;


void setup() 
{  
  //configre the GPIO pins
  pinMode(PIN_LED_TX, OUTPUT);  // general purpose LED
  pinMode(Release_B, OUTPUT);    //for Endurosat antenna
  pinMode(Release_A, OUTPUT);    //for Endurosat antenna
  pinMode(Current_5V, INPUT);    //Analog signal that should be proportional to 5V current
  pinMode(TX_RX, OUTPUT);        // TX/ RX-bar
  pinMode(RX_TX, OUTPUT);        // RX/ TX-bar
  pinMode(PAENABLE, OUTPUT);     //enable the PA
  pinMode(EN0, OUTPUT);          //enable serial port differential driver
  pinMode(EN1, OUTPUT);          //enable serial port differential driver
  pinMode(AX5043_DCLK, INPUT);   //clock from the AX5043 when using wire mode
  pinMode(AX5043_DATA, OUTPUT);  //data to the AX5043 when using wire mode
  pinMode(OC3V3, INPUT);         //kind of a useless signal that indicates that there is an overcurrent on the 3V3 (our own supply)
  pinMode(OC5V, INPUT);          //much more useful indication of an over current on the 5V supply
  pinMode(SELBAR, OUTPUT);       //select for the AX5043 SPI bus
  pinMode(SYSCLK, INPUT);        //AX5043 crystal oscillator clock output

  //set the default state (Receiver on, PA off)
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);
  digitalWrite(PAENABLE, LOW);
  digitalWrite(PIN_LED_TX, LOW);  //outputs a high while in transmit mode

  //set the data pin for wire mode into the AX5043 low, NOT transmitting
  digitalWrite(AX5043_DATA, LOW);

  //enable the differential serial port drivers (Silversat board only)
  digitalWrite(EN0, HIGH);
  digitalWrite(EN1, HIGH);

  //start the I2C interface and the serial ports
  Wire.begin();

  Serial.begin(115200);
  while (!Serial) {};
  Serial1.begin(19200);  //I repeat...Serial 1 is Payload (RPi)
  while (!Serial1) {};
  Serial0.begin(57600);  //I repeat...Serial 0 is Avionics
  //while(!Serial0) {};  //taken out or we're waiting for a port we're not testing at the moment

  //serial port roll call
  Serial.println("I'm Debug");
  Serial1.println("I'm Payload");
  Serial0.println("I'm Avionics");

  //start SPI, configure and start up the radio
  debug_printf("starting up the radio\n");
  SPI.begin();
  SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));  //these settings seem to work, but not optimized

  //create an array in memory to hold the ax5043 configuration
  memset(&config, 0, sizeof(ax_config));

  // ------- init -------
  //the headings below corresponds to the main parts of the config structure. Unless changed they are loaded with defaults

  /* power mode */
  //generally handled internally, so consider it a variable handled by a private function

  /* synthesiser */
  config.synthesiser.vco_type = AX_VCO_INTERNAL;  //note: I added this to try to match the DVK, this means that the external inductor is not used
  config.synthesiser.A.frequency = 433000000;
  config.synthesiser.B.frequency = 433000000;

  /* external clock */
  config.clock_source = AX_CLOCK_SOURCE_TCXO;
  config.f_xtal = 48000000;

  /* transmit path */
  //default is differential; needs to be single ended for external PA; NOTE: there is a command to change the path
  config.transmit_power_limit = 1;

  /* SPI transfer */
  config.spi_transfer = wiring_spi_transfer;  //define the SPI handler

  /* receive */
  //config.pkt_store_flags = AX_PKT_STORE_RSSI | AX_PKT_STORE_RF_OFFSET;  //search on "AX_PKT_STORE" for other options, only data rate offset is implemented
  //config.pkt_accept_flags =     //configures what types of packets (as indicated by the flags on the received chunk) to accept.

  /* wakeup */
  //for WOR, we're not using

  /* digital to analogue (DAC) channel */
  //not needed

  /* PLL VCO */
  //frequency range of vco; see ax_set_pll_parameters
  // ------- end init -------

  ax_init(&config);  //this does a reset, so needs to be first

  //load the RF parameters for the current config
  ax_default_params(&config, &fsk_modulation);  //ax_modes.c for RF parameters

  //parrot back what we set
  debug_printf("config variable values: \n");
  debug_printf("tcxo frequency: %u \n", uint(config.f_xtal));
  debug_printf("synthesizer A frequency: %u \n", uint(config.synthesiser.A.frequency));
  debug_printf("synthesizer B frequency: %u \n", uint(config.synthesiser.B.frequency));
  debug_printf("status: %x \n", ax_hw_status());
  
  //setup the PA temp sensor
  float patemp = tempsense.readTemperatureC();
  debug_printf("temperature of PA: %.1f \n", patemp);

  //turn on the receiver
  ax_rx_on(&config, &fsk_modulation);

  //for debugging
  //printRegisters();

  //for measuring loop timing
  //lastlooptime = micros();
}

void loop() 
{
  //the first thing you'll want to do is to create a single tone ("dead carrier") at our target frequency
  //this is much like how we'll create beacons
  //first you put the chip into WIRE mode (we use PACKET mode normally).  Here's the procedure:

  /* 1.  Switch the AX5043 into WIRE mode using ASK modulation.  When in WIRE mode
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
  */
  //note that in the setup we've set the chip up for fsk modulation.  For this test we're going to use ASK modulation

  //first reset the chip
  ax_init(&config);  //this may not be needed

  //load the RF parameters
  ax_default_params(&config, &ask_modulation);
  debug_printf("ASK params loaded \n");

  //pinfunc_t is an enum, so we have to define using that datatype because that's what the ax_set_pinfunc_data function expects.
  pinfunc_t func = 0x84;
  ax_set_pinfunc_data(&config, func);
  debug_printf("WIRE mode enabled \n");

  //set the RF switch to transmit
  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);
  debug_printf("RF switch configured for transmit \n");

  ax_tx_on(&config, &ask_modulation);

  //we're now going to turn the radio on and off (to keep it from getting very hot) at a low enough rate to allow us to see the output
  //note that we're not reconfiguring the RF switch.  This might be needed later if there is any spurious output from the transmitter when off
  while (true) 
  {  
    //turn the PA on
    digitalWrite(PAENABLE, HIGH);
    digitalWrite(PIN_LED_TX, HIGH);

    //now set the data pin on the AX5043 high to indicate a "1" (transmitting)
    digitalWrite(AX5043_DATA, HIGH);

    //leave it on for 2 seconds
    delay(2000);  //consider replacing this with a #define BIT_TIME..do you see a path to create beacons?

    //now turn it off
    digitalWrite(AX5043_DATA, LOW);
    digitalWrite(PAENABLE, LOW); //turn off the PA
    digitalWrite(PIN_LED_TX, LOW);
    
    //leave it off for 2 seconds
    delay(2000);  
  }
}

//wiring_spi_transfer defines the chip selects on the SPI bus
void wiring_spi_transfer(unsigned char *data, uint8_t length) {
  digitalWrite(SELBAR, LOW);   //select
  SPI.transfer(data, length);  //do the transfer
  digitalWrite(SELBAR, HIGH);  //deselect
}