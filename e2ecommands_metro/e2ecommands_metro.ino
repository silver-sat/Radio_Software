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
 * Unless you're working with a Metro, in which case you need 2 USB to TTL Serial (3.3V).
 * 
 *
 * debug output goes to Serial
 *
 * NOTE: there may be no need to switch PA path.  It's handled internally, diff for RX, se for TX
 * ALSO NOTE: you must use HDLC to use FEC
 */

//#define DEBUG

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

//this define is required by AX library.  Use DIFF for eval boards, SE for Silversat boards.  It tells which radio transmit path to use.
#define _AX_TX_DIFF
//#define _AX_TX_SE

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
#include "constants.h"

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
#define TXBUFFSIZE 1024 //at most 2 packets

// delay values for time to wait after setting T/R lines and turning on PA, and time to allow PA to stabilize.
//This is a guess at the moment and probably way too large.

//these constants now defined in constants.cpp
//pa_delay is time to wait after switching on the PA
//tx_delay is the delay before switching from RX to TX.  Once it switches, it sends a packet immediately.  
//clear_threshold is the threshold to declare the channel clear

CircularBuffer<unsigned char, CMDBUFFSIZE> cmdbuffer;
CircularBuffer<unsigned char, DATABUFFSIZE> databuffer;
CircularBuffer<unsigned char, TXBUFFSIZE> txbuffer;

int cmdpacketsize {0};  //really the size of the first packet in the buffer  Should think about whether or not these could be local vs. global
int datapacketsize {0};
int txbufflen {0}; //size of next packet in buffer

//radio config and interface
ax_packet rx_pkt;  //instance of packet structure

//radio config
ax_config config;

//one state variable
bool transmit {false};  // by default, we are not transmitting; might use the other bits in this for FIFO flags?

//temperature sensor
Generic_LM75_10Bit tempsense(0x4B);

//timing 
unsigned int lastlooptime {0};  //for timing the loop (debug)
unsigned int rxlooptimer {0};  //for determining the delay before switching modes (part of CCA)

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
  //while (!Serial) {};
  Serial1.begin(19200);  //I repeat...Serial 1 is Payload (RPi)
  //while (!Serial1) {};
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

  //fill the ax5043 config array with zeros
  memset(&config, 0, sizeof(ax_config));

  // ------- init -------
  //the headings below corresponds to the main parts of the config structure. Unless changed they are loaded with defaults
  /* power mode */
  //generally handled internally, so consider it a variable handled by a private function

  /* synthesiser */
  config.synthesiser.vco_type = AX_VCO_INTERNAL;  //note: I added this to try to match the DVK, this means that the external inductor is not used
  config.synthesiser.A.frequency = constants::frequency;
  config.synthesiser.B.frequency = constants::frequency;

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

  //parrot back what we set
  debug_printf("config variable values: \n");
  debug_printf("tcxo frequency: %d \n", int(config.f_xtal));
  debug_printf("synthesizer A frequency: %d \n", int(config.synthesiser.A.frequency));
  debug_printf("synthesizer B frequency: %d \n", int(config.synthesiser.B.frequency));
  debug_printf("status: %x \n", ax_hw_status());
  
  //setup the PA temp sensor
  float patemp = tempsense.readTemperatureC();
  debug_printf("temperature of PA: %.1f \n", patemp);

  //total free memory after setup
  debug_printf("free memory %d \n", freeMemory());

  //turn on the receiver
  ax_rx_on(&config, &fsk_modulation);

  //for debugging
  //printRegisters();

  //for measuring loop timing
  //lastlooptime = micros();
}


//the main loop consists of three main parts; an interface handler, a transmit handler and a receive handler.
//this loop needs to be tight and generally non-blocking, because we are polling the radio
void loop() 
{
  //debug_printf("%x \n", micros() - lastlooptime);
  //lastlooptime = micros();
  
  //interface handler - the interface handler processes packets coming in via the serial interfaces.
  //for incoming data from the GS or avionics, just stick it into the circular buffer
  //an ALTERNATIVE would be to identify a packet as it comes in (look for C0), and then store it into a Packet object (inside the circular buffer).
  //you'd have to KISS decode on the fly, which might not be all that difficult (cmds are really easy, ASCII only, there are no escape characters)
  //there is an advantage that you don't need to process prior to transmit..just grab the next object in the buffer and ship it out.
  //it also might allow some direct processing of the packet data.  But remember that data is coming in fast (about 87 uS per byte @ 115.2k) leaving only so many
  //clock cycles to do the processing (about 3.8k cycles per byte), but this could be efficient 
  if (Serial0.available() > 0) 
  {
    cmdbuffer.push(Serial0.read());  //we add data coming in to the tail...what's at the head is the oldest packet
    if (cmdbuffer.isFull()) 
    {
      debug_printf("CMD BUFFER OVERFLOW \n");  //to date, have never seen this (or the data version) ever happen.
    }
  }

  //data, put it into its own buffer
  if (Serial1.available() > 0) 
  {
    databuffer.push(Serial1.read());  //we add data coming in to the tail...what's at the head is the oldest packet
    if (databuffer.isFull()) 
    {
      debug_printf("DATA BUFFER OVERFLOW \n");
    }
  }

  //process the command buffer first - processbuff returns the size of the next packet in the buffer, returns 0 if none
  cmdpacketsize = processbuff(cmdbuffer);  //really the size of the first packet in the buffer

  //process the databuffer - see note above about changing the flow
  datapacketsize = processbuff(databuffer);

 //-------------end interface handler--------------

 //------------begin data processor----------------

  if (cmdpacketsize != 0)  //only run this if there is a complete packet in the buffer
  {
    processcmdbuff(cmdbuffer, databuffer, cmdpacketsize, config);
    //processbuff(cmdbuffer);  //process buff is blocking and empties the cmd buffer --why is this here? for more than one command?, then it's wrong
  }

  //prepare a packet for transmit; the transmit loop will reset txbufflen to 0 after transmitting the buffer
  if (datapacketsize != 0)
  {  
    if (txbuffer.size() == 0) //just doing the next packet to keep from this process from blocking too much
    { 
      unsigned char kisspacket[1027];  //allow for a very big kiss packet, probably overkill (abs max is, now 512 x 2 + 3)
      unsigned char nokisspacket[512]; //should be just the data
      //debug_printf("pulling kiss formatted packet out of databuffer and into kisspacket \n");
      //note this REMOVES the data from the databuffer...no going backsies
      for (int i = 0; i < datapacketsize; i++) 
      {
        kisspacket[i] = databuffer.shift();
        //debug_printf("kisspacket %x  |  %x \n", i, kisspacket[i]);
      }
      //debug_printf("removing KISS \n");  //kiss_unwrap returns the size of the new buffer
      txbufflen = kiss_unwrap(kisspacket, datapacketsize, nokisspacket);
      for (int i=0; i< txbufflen; i++)
      {
        txbuffer.push(nokisspacket[i]); //push the unwrapped packet onto the tx buffer
      }
    }
  }    
  //-------------end data processor---------------------

  //transmit handler - the transmit handler processes data in the buffers when the radio is in transmit mode
  if (transmit == true) 
  {
    if (datapacketsize == 0 && txbuffer.size() == 0) //datapacketsize should still be nonzero until the buffer is processed again (next loop)
    {
      transmit = false;                     //change state and we should drop out of loop
      while (ax_RADIOSTATE(&config)) {};    //check to make sure all outgoing packets are done transmitting
      set_receive(config, fsk_modulation);  //this also changes the config parameter for the TX path to differential
      debug_printf("State changed to FULL_RX \n");
      //debug_printf("free memory %d \n", freeMemory());
    }
    else if (ax_RADIOSTATE(&config) == 0)  //radio is idle, so we can transmit a packet, keep this non-blocking if it's active so we can process the next packet
    {    
      debug_printf("transmitting packet \n");
      debug_printf("txbufflen: %x \n", txbufflen);
      unsigned char txqueue[512];
      for (int i=0; i<txbufflen; i++)  //clear the transmitted packet out of the buffer and stick it in the txqueue
      //we had to do this because txbuffer is of type CircularBuffer, and ax_tx_packet is expecting a pointer.
      //might change this to store pointers in the circular buffer (see object handling in Circular Buffer reference)
      //and just create an array of stored packets
      //TODO: alternatively see if this compiles without recasting the txbuffer and passing it directly.
      {
        txqueue[i] = txbuffer.shift();
      }
      digitalWrite(PIN_LED_TX, HIGH); 
      ax_tx_packet(&config, &fsk_modulation, txqueue, txbufflen);  //transmit the decoded buffer, this is blocking except for when the last chunk is committed.
      //this is because we're sitting and checking the FIFCOUNT register until there's enough room for the final chunk.
      
      digitalWrite(PIN_LED_TX, LOW);  
    }   
  }  
  //-------------end transmit handler--------------

  //receive handler
  if (transmit == false) {
    if (ax_rx_packet(&config, &rx_pkt))  //the FIFO is not empty...there's something in the FIFO and we've just received it
    {
      unsigned char rxpacket[1026];  //this is the KISS encoded received packet, 2x max packet size plus 2
      debug_printf("got a packet! \n");
      int rxpacketlength = 0;
      rxlooptimer = micros();
      //if it's HDLC, then the "address byte" (actually the KISS command byte) is in rx_pkt.data[1], because there's no length byte
      //by default we're sending out data, if it's 0xAA, then it's a command
      
      //HDLC does NOT include the length byte, so you need to take that into account.  
      //HDLC also always stores the CRC bytes, but I'm not sure why there are four of them. See pg. 71 of prog manual..  
      //So in this case we want the first byte (yes, we do) and we don't want the last 2 (for CRC-16..which hdlc has left for us)
      //we also need to put back the command byte      
      rxpacketlength = kiss_encapsulate(rx_pkt.data, rx_pkt.length-2, rxpacket);  //remove the 2 extra bytes from the received packet length    
             
      if (rx_pkt.data[0] != 0xAA)  //packet.data is type unsigned char
      {                    //there are only 2 endpoints, data (Serial1) or command responses (Serial0), rx_pkt is an instance of the ax_packet structure that includes the metadata
        Serial1.write(rxpacket, rxpacketlength);  //so it's data..send it to payload or to the proxy
      } 
      else {        
        Serial0.write(rxpacket, rxpacketlength);  //so it's a command response , assumption is first byte of command or response is a zero..indicating that it goes to Avionics.  This can be replaced with something more complex
      }
      
    } 
    else { //the fifo is empty
      //printf("delay to enter state switch: %x", ((micros() - rxlooptimer) > 1000));
      //unsigned char rssi = ax_RSSI(&config);
      //unsigned char bgndrssi = ax_BGNDRSSI(&config);
      //unsigned char fifostat = ax_FIFOSTAT(&config);
      //printf("current fifo status: %x \n", fifostat);
      int firstrssi = ax_RSSI(&config);  //just trying a simple average over 2 slightly separated readings
      int secondrssi = 0x0FFF;
      int avgrssi = (firstrssi + secondrssi)/2;
      bool channelclear = false;
      if ((micros() - rxlooptimer) > constants::tx_delay)  //intent here is for average to be high until enough time has passed to get second reading, and then allow rest of loop to continue
      {
        secondrssi = ax_RSSI(&config);  //now take a sample
        avgrssi = (firstrssi + secondrssi)/2;  //and compute a new average
        if (avgrssi > constants::clear_threshold)
        {
          rxlooptimer = micros();
          channelclear = false; 
          //printf("channel not clear");          
        }
        else 
        {
          channelclear = true;
        }
      }    
      //printf("avgrssi: %x \n", avgrssi);
      //so, we might have something in the tx buffer, but we're not quite ready to transmit.  
      //add a non-blocking delay before changing mode
      //so we probably don't want to check if the channel is clear until we're really ready to transmit.
      //I moved the code to calculate the next tx buffer to the main loop...
      if ((datapacketsize != 0) && channelclear == true) 
      {  //there's something in the tx buffers and the channel is clear
        transmit = true;
        //printf("avgRSSI = %x \n", avgrssi);
        printf("delay %lu \n", micros() - rxlooptimer);
        rxlooptimer = micros();
        //ax_off(&config);
        set_transmit(config, fsk_modulation);  //this also changes the config parameter for the TX path to single ended
        debug_printf("State changed to FULL_TX \n");
        //debug_printf("free memory %d \n", freeMemory());
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

//setup the radio for transmit.  Set the TR lines (T/~R and R/~T) to Transmit state, set the AX5043 tx path, and enable the PA
void set_transmit(ax_config& config, ax_modulation& mod) {
  ax_set_pwrmode(&config, 0x05);  //see errata
  ax_set_pwrmode(&config, 0x07);  //see errata
  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);  
  //debug_printf("changing tx path to single ended \n");
  //ax_set_tx_path(&config, AX_TRANSMIT_PATH_SE);  // this should be immediate
  digitalWrite(PAENABLE, HIGH);                  // enable the PA BEFORE turning on the transmitter
  delayMicroseconds(constants::pa_delay);
  //ax_init(&config);                              //this and the next line might not be necessary
  //ax_default_params(&config, &mod);              //this just reloads the parameters, so it might not be necessary
  ax_tx_on(&config, &mod);                       //turn on the radio in full tx mode
  //digitalWrite(PIN_LED_TX, HIGH);   
}


//setup the radio for receive.  Set the TR lines (T/~R and R/~T) to Receive state, un-set the AX5043 tx path, and disable the PA
void set_receive(ax_config& config, ax_modulation& mod) {
  //ax_init(&config);
  //ax_default_params(&config, &mod);                       //load the new parameters (differential)..note that this step might not be necessary
  ax_rx_on(&config, &mod);                     //go into full_RX mode
  //digitalWrite(PIN_LED_TX, LOW);
  digitalWrite(PAENABLE, LOW);  //cut the power to the PA
  delayMicroseconds(constants::pa_delay);               //wait for it to turn off
  //debug_printf("changing tx path to differential \n");
  //ax_set_tx_path(&config, AX_TRANSMIT_PATH_DIFF);  //change the path back to differential...this should be immediate, but I'm reloading anyway?
  digitalWrite(TX_RX, LOW);                        //set the TR state to receive
  digitalWrite(RX_TX, HIGH);
  
}

void printRegisters(){
    printf("Here are the AX5043 register contents \n");

  // this prints all but ADC, low power oscillator and DAC registers

  printf("registers below 0x70, general config \n");
  for (uint16_t reg = 0; reg < 0x70; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameters \n");
  // range 0x100 to 0x118
  for (uint16_t reg = 0x100; reg < 0x119; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 0 \n");
  // range 0x120 to 0x12F
  for (uint16_t reg = 0x120; reg < 0x130; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 1 \n");
  // range 0x130 to 0x13F
  for (uint16_t reg = 0x130; reg < 0x140; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 2 \n");
  // range 0x140 to 0x14F
  for (uint16_t reg = 0x140; reg < 0x150; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 3 \n");
  // range 0x150 to 0x15F
  for (uint16_t reg = 0x150; reg < 0x160; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("transmitter parameters \n");
  // range 0x160 to 0x171
  for (uint16_t reg = 0x160; reg < 0x172; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("pll and crystal parameters \n");
  // range 0x180 to 0x189
  for (uint16_t reg = 0x180; reg < 0x18A; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("packet format \n");
  // range 0x200 to 0x20B
  for (uint16_t reg = 0x200; reg < 0x20C; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("pattern match \n");
  // range 0x210 to 0x21E
  for (uint16_t reg = 0x210; reg < 0x21F; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("packet controller \n");
  // range 0x220 to 0x233
  for (uint16_t reg = 0x220; reg < 0x234; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("special functions \n");
  // 0x248, 24A, 250, 255
    printf("register= %2x | value= %2x \n", 0x248,
           ax_hw_read_register_8(&config, 0x248));
    printf("register= %2x | value= %2x \n", 0x24A,
           ax_hw_read_register_8(&config, 0x24A));
    printf("register= %2x | value= %2x \n", 0x250,
           ax_hw_read_register_8(&config, 0x250));
    printf("register= %2x | value= %2x \n", 0x255,
           ax_hw_read_register_8(&config, 0x255));

  printf("ADC & low power oscillator \n");
  // range 0x300 to 0x332
  for (uint16_t reg = 0x300; reg < 0x332; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("peformance tuning registers \n");
  // range 0xF00 to 0xFFF
  for (uint16_t reg = 0xF00; reg <= 0xFFF; reg++) {
    printf("register= %2x | value= %2x \n", reg,
           ax_hw_read_register_8(&config, reg));
  }
}

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}