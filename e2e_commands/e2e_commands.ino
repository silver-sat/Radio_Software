/**
 * @file e2e_commands.ino
 * @author Tom Conrad (tom@silversat.org)
 * @brief end to end commands using Silversat radio board
 * @version 1.0.1
 * @date 2022-11-08
 *
 * What we're testing:
 * Correct command routing
 * Correct command interpretation and packet handling
 * Correct command protocol (command, ACK/NACK, Action, Response)
 * Correct response format
 * Correct action
 * Correct buffer handling
 * Radio operation
 * Kiss decoding/encoding
 *
 * This version is all functions and does not try to make anything like pretty C++ classes.  It just works.
 * Commands are the easiest to convert into a class.  Other potential classes: Packet (to combine Kiss and packet functions), 
 * Beacon (or move this into command), Antenna, and Status(?, not sure about that one, they're all 'actions')
 *
 * Serial 2 is no longer needed.  beacons are issued by changing the radio state to wire mode using ASK
 * Serial 0 represents the Ground station or Avionics.  Commands and remote Command responses are sent via Serial0 (as data), and local responses are issued to Serial0 by the radio board.
 * Serial 1 is the data port.  The intent is for this to almost seem like a packet pass-through. (essentially one of the tests)
 *
 *
 * 2 RS422/RS485 converters are required to run the test PER SYSTEM, but 2 total will do at first since we're not really trying to pass data just yet (although it should work).
 * There is also a *very* rudimentary python script that issues commands (canned) and listens for responses.
 * 
 *
 * debug output goes to Serial
 */

#define DEBUG

//this define is required by AX library.  
#define _AX_TX_DIFF
#define _AX_TX_SE

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#include <CircularBuffer.h>
#include <LibPrintf.h>
#include <SPI.h>
#include <Wire.h>
#include <Temperature_LM75_Derived.h>

//custom local files
#include "packetfinder.h"
#include "commands.h"
#include "KISS.h"

//the AX library files
#include "ax.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"


#define CMDBUFFSIZE 512  //this buffer can be smaller because we control the rate at which packets come in
#define DATABUFFSIZE 8192  //how many packets do we need to buffer at most during a TCP session?

// delay values for time to wait after setting T/R lines and turning on PA, and time to allow PA to stabilize.
//This is a guess at the moment and probably way too large.
#define PAdelay 1

CircularBuffer<unsigned char, CMDBUFFSIZE> cmdbuffer;
//CircularBuffer<unsigned char, DATABUFFSIZE> databuffer;
CircularBuffer<unsigned char, DATABUFFSIZE> txbuffer;

uint16_t nextCmdPacketSize;  //really the size of the first packet in the buffer
uint16_t nextDataPacketSize;

//radio config and interface
ax_packet rx_pkt;  //instance of packet structure

//radio config
ax_config config;

//one state variable
bool transmit {false};  // by default, we are not transmitting; might use the other bits in this for FIFO flags?

//temperature sensor
Generic_LM75_10Bit tempsense(0x4B);

void setup() {
  pinMode(PIN_LED_TXL, OUTPUT);  // general purpose LED
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

  //default RX mode state
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);
  digitalWrite(PAENABLE, LOW);

  //set the data pin for wire mode into the AX5043 low, NOT transmitting
  digitalWrite(AX5043_DATA, LOW);

  //enable the differential serial port drivers
  digitalWrite(EN0, HIGH);
  digitalWrite(EN1, HIGH);

  //start the I2C interface and the serial ports
  Wire.begin();

  Serial.begin(57600);
  while (!Serial) {};
  Serial1.begin(57600);
  while (!Serial1) {};
  Serial0.begin(57600);
  //while(!Serial0) {};  //taken out or we're waiting for a port we're not testing at the moment

  Serial.println("I'm Serial");
  Serial1.println("I'm Serial 1");
  Serial0.println("I'm Serial 0");

  //start SPI and start up the radio
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

  /* SPI transfer */
  config.spi_transfer = wiring_spi_transfer;  //define the SPI handler

  /* receive */
  config.pkt_store_flags = AX_PKT_STORE_RSSI | AX_PKT_STORE_RF_OFFSET;  //search on "AX_PKT_STORE" for other options, only data rate offset is implemented
  // config.pkt_accept_flags =     // code sets accept residue, accept bad address, accept packets that span fifo chunks.  We DON'T want to accept residues, or packets that span more than one

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

  //ax_set_pinfunc_pwramp() // default should be okay "0110" on reset, which should be output pwr amp control
  //NOTE: this signal is kind of not needed because we know that we need to turn on the PA and change the signal path
  //whenever we change mode anyway.  This would make a lot more sense if the chip automatically changed mode.  But it doesn't.

  //parrot back what we set
  debug_printf("config variable values: \n");
  debug_printf("tcxo frequency: %u \n", config.f_xtal);
  debug_printf("synthesizer A frequency: %u \n", config.synthesiser.A.frequency);
  debug_printf("synthesizer B frequency: %u \n", config.synthesiser.B.frequency);
  debug_printf("status: %x \n", ax_hw_status());

  debug_printf("temperature of PA: %s \n", tempsense.readTemperatureC());

  //turn on the receiver
  ax_rx_on(&config, &fsk_modulation);
}


  //the main loop consists of three main parts; an interface handler, a transmit handler and a receive handler.
  //this loop needs to be tight and generally non-blocking, because we are polling the radio
void loop() {
  //interface handler - the interface handler processes packets coming in via the serial interfaces.
  //for incoming data from the GS or avionics, just stick it into the circular buffer
  if (Serial0.available() > 0) {
    cmdbuffer.push(Serial0.read());  //we add data coming in to the tail...what's at the head is the oldest packet
    if (cmdbuffer.isFull()) debug_printf("CMD BUFFER OVERFLOW \n");
  }

  //for data, put it into its buffer.  Later, fix this to go to databuffer instead.  Why?, because if we get a halt, 
  //we need to push that into the TX queue first.  HOWEVER, there really is no need to be gentle about this.  The data
  //in the buffer is trash.  We can just shove the command into the buffer.  Even if we split up a data packet, it really doesn't matter.
  //sooo, there may be no need for the databuffer after all...

  if (Serial1.available() > 0) {
    txbuffer.push(Serial1.read());  //we add data coming in to the tail...what's at the head is the oldest packet
    if (txbuffer.isFull()) debug_printf("DATA BUFFER OVERFLOW \n");
  }

  //process the command buffer first - processbuff returns the size of the next packet in the buffer, returns 0 if none
  nextCmdPacketSize = processbuff(cmdbuffer);

  if (nextCmdPacketSize != 0)  //only run this if there is a complete packet in the buffer
  {
    processcmdbuff(cmdbuffer, txbuffer, nextCmdPacketSize, config);
    processbuff(cmdbuffer);  //process buff is blocking and empties the cmd buffer
  }

  //process the txbuffer - see note above about changing the flow
  nextDataPacketSize = processbuff(txbuffer);
  //-------------end interface handler--------------

  //transmit handler - the transmit handler processes data in the buffers when the radio is in transmit mode
  if (transmit == true) {
    if (nextDataPacketSize != 0) {
      //send that packet out Serial 2 and remove it from the buffer; this will keep removing packets as long as there's nothing in the command queue
      unsigned char kissoutdatapacket[514];  //allow for a very big kiss packet, probably overkill (abs max is 256 x 2 + 2)
      debug_printf("pulling kiss formatted packet out of txbuffer and into outkiss \n");
      for (int i = 0; i < nextDataPacketSize; i++) {
        kissoutdatapacket[i] = txbuffer.shift();
      }
      unsigned char radiopacket[256];
      debug_printf("removing KISS \n");  //kiss_unwrap returns the size of the new buffer
      int nokissbufflen = kiss_unwrap(kissoutdatapacket, nextDataPacketSize, radiopacket);

      //so what's really coming out of the kiss unwrapper?  it should lose the C0's and the first command byte, but does it? nope, the command byte is considered part of the packet.  So i need to strip it out.
      unsigned char *outdata;
      outdata = radiopacket + 1;  //this is a hack...I don't want the command byte to get retransmitted, so I'm dropping it by creating a new pointer that's at the next address.

      debug_printf("data packet going out \n");
      for (int i = 0; i < nokissbufflen - 1; i++) {
        debug_printf("value: %x , data: %x \n", i, outdata[i]);
      }

      debug_printf("transmitting packet \n");
      ax_tx_packet(&config, &fsk_modulation, outdata, nokissbufflen - 1);  //transmit the decoded buffer
      while (ax_RADIOSTATE(&config) != 0) {};                              //that means something is being transmitted...RADIOSTATE is an AX5043 variable
    }
    
    //nextDataPacketSize = processbuff(txbuffer);  //consider removing this when comfortable with queuing
    //debug_printf("new size of tx buffer: %x \n", nextDataPacketSize);
    //debug_printf("new size of cmd buffer: %x \n", nextCmdPacketSize);

    if (nextCmdPacketSize == 0 && nextDataPacketSize == 0) {
      transmit = false;                     //change state and we should drop out of loop
      while (ax_RADIOSTATE(&config)) {};    //check to make sure all outgoing packets are done transmitting
      ax_off(&config);
      set_receive(config, fsk_modulation);  //this also changes the config parameter for the TX path to differential
      debug_printf("State changed to FULL_RX \n");
    }
  }
  //-------------end transmit handler--------------

  //receive handler
  if (transmit == false) {
    if (ax_rx_packet(&config, &rx_pkt))  //the FIFO is not empty...there's something in the FIFO and we've just received it
    {
      unsigned char encodedRadioPacket[514];  //this is the KISS encoded received packet
      int encodedRadioPacketLength = kiss_encapsulate(rx_pkt.data, rx_pkt.length, encodedRadioPacket);  //this is the length after KISS encapsulation
      if (rx_pkt.data[0])  //packet.data is type unsigned char
      {                    //there are only 2 endpoints, data (Serial1) or command responses (Serial0), rx_pkt is an instance of the ax_packet structure that includes the metadata
        Serial0.write(encodedRadioPacket, encodedRadioPacketLength);  //so it's data..send it to payload or to the proxy
      } 
      else {        
        Serial1.write(encodedRadioPacket, encodedRadioPacketLength);  //so it's a command response , assumption is first byte of command or response is a zero..indicating that it goes to Avionics.  This can be replaced with something more complex
      }
    } else {
      if (nextDataPacketSize != 0) {  //there's something in the tx buffers so switch mode
        transmit = true;
        ax_off(&config);
        set_transmit(config, fsk_modulation);  //this also changes the config parameter for the TX path to single ended
        debug_printf("State changed to FULL_TX \n");
      }
    }
  }
  //-------------end receive handler--------------
}
//-------------end loop--------------


//wiring_spi_transfer defines the chip selects on the SPI bus
void wiring_spi_transfer(unsigned char *data, uint8_t length) {
  digitalWrite(SELBAR, LOW);   //select
  SPI.transfer(data, length);  //do the transfer
  digitalWrite(SELBAR, HIGH);  //deselect
}


void printbuff(CircularBuffer<unsigned char, CMDBUFFSIZE>& mybuffer) {
  debug_printf("Here's the contents of the command buffer\n");
  if (mybuffer.size() == 0) {
    debug_printf("Buffer is empty \n");
  } else {
    for (int i = 0; i < mybuffer.size(); i++) {
      debug_printf("index %x , value %x \n", i, mybuffer[i]);
    }
  }
}

//setup the radio for transmit.  Set the TR lines (T/~R and R/~T) to Transmit state, set the AX5043 tx path, and enable the PA
void set_transmit(ax_config& config, ax_modulation& mod) {
  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);
  debug_printf("changing tx path to single ended \n");
  ax_set_tx_path(&config, AX_TRANSMIT_PATH_SE);  // this should be immediate
  digitalWrite(PAENABLE, HIGH);                  // enable the PA BEFORE turning on the transmitter
  delay(PAdelay);
  ax_default_params(&config, &mod);              //this just reloads the parameters, so it might not be necessary
  ax_tx_on(&config, &mod);                       //turn on the radio in full tx mode
  digitalWrite(PIN_LED_TXL, HIGH);   
}


//setup the radio for receive.  Set the TR lines (T/~R and R/~T) to Receive state, un-set the AX5043 tx path, and disable the PA
void set_receive(ax_config& config, ax_modulation& mod) {
  ax_default_params(&config, &mod);                       //load the new parameters (differential)..note that this step might not be necessary
  ax_rx_on(&config, &fsk_modulation);                     //go into full_RX mode
  digitalWrite(PIN_LED_TXL, LOW);
  digitalWrite(PAENABLE, LOW);  //cut the power to the PA
  delay(PAdelay);               //wait for it to turn off
  debug_printf("changing tx path to differential \n");
  ax_set_tx_path(&config, AX_TRANSMIT_PATH_DIFF);  //change the path back to differential...this should be immediate, but I'm reloading anyway?
  digitalWrite(TX_RX, LOW);                        //set the TR state to receive
  digitalWrite(RX_TX, HIGH);
  
}