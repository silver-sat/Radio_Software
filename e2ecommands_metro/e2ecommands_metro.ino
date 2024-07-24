/**
 * @file e2e_commands.ino
 * @author Tom Conrad (tom@silversat.org)
 * @brief end to end commands using Silversat radio board
 * @version 1.0.1
 * @date 2022-11-08
 *
 *
 * Serial 2 is no longer needed.  beacons are issued by changing the radio state to wire mode using ASK
 * Serial 0 represents the Ground station or Avionics.  Commands and remote Command responses are sent via Serial0 (as data), and local responses are issued to Serial0 by the radio board.
 * Serial 1 is the data port.  The intent is for this to almost seem like a packet pass-through. (essentially one of the tests)
 *
 *
 * 2 RS422/RS485 converters are required to run the test PER SYSTEM, but 2 total will do at first since we're not really trying to pass data just yet (although it should work).
 * There is also a python script with GUI that issues commands (canned) and listens for responses.
 * Unless you're working with a Metro, in which case you need 2 USB to TTL Serial (3.3V).
 *
 *
 * debug output goes to Serial
 *
 * NOTE: there is no need to switch PA path.  It's handled internally, diff for RX, se for TX, but make sure you have the right #define in ax.cpp
 * ALSO NOTE: you must use HDLC to use FEC
 *
 * constants are now defined in constants.cpp
 * some of these are a guess at the moment and probably way too large.
 * delay values are available for the time to wait after setting T/R lines and time to allow PA to stabilize
 * pa_delay is time to wait after switching on the PA
 * tx_delay is the delay before switching from RX to TX.  Once it switches, it sends a packet immediately.
 * clear_threshold is the threshold to declare the channel clear
 * mtu_size is the mtu size defined in tnc attach.  There are 4 additional bytes for the TUN interface header.  That is all transmitted, since it's within the KISS frame on Serial interface
 * The serial interface is KISS encoded.  When prepped for transmit the processor removes the KISS formatting, but retains the KISS command byte
 * The radio adds 2 additional CRC bytes (assuming we're using CRC-16)
 * when received by the remote radio, and the data is handed to the processor, we remove the CRC bytes and then reapply KISS encoding prior to writing to the appropriate port.

 * the main loop consists of three main parts; an interface handler, a transmit handler and a receive handler.
 * the loop needs to be tight and generally non-blocking because we are polling the radio (not using interrupts for now)

 * interface handler - the interface handler processes packets coming in via the serial interfaces.
 * for incoming data from the GS or avionics, just stick it into the circular buffer
 * an ALTERNATIVE would be to identify a packet as it comes in (look for C0), and then store it into a Packet object (inside the circular buffer).
 * you'd have to KISS decode on the fly, which might not be all that difficult (cmds are really easy, ASCII only, there are no escape characters)
 * there is an advantage that you don't need to process prior to transmit..just grab the next object in the buffer and ship it out.
 * it also might allow some direct processing of the packet data.  But remember that data is coming in fast (about 87 uS per byte @ 115.2k) leaving only so many
 * clock cycles to do the processing (about 3.8k cycles per byte), but this could be efficient
 *
 * TODO:  1. Determine if we want to accept multiple chunks.  Roughly line 1143 in ax.cpp
 *        //AX_PKT_ACCEPT_MULTIPLE_CHUNKS |  // (LRGP) // // tkc - no longer accepting multiple chunks.
 *        2. 

*/

//#define DEBUG
#define _RADIO_BOARD_  //this is needed for variant file...see variant.h
#define SERIAL_BUFFER_SIZE 512

/*
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
*/

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#define CIRCULAR_BUFFER_INT_SAFE

#include <CircularBuffer.hpp>
#include <LibPrintf.h>
#include <SPI.h>
#include <Wire.h>


//custom local files
#include "packetfinder.h"
//#include "commands.h"
#include "commands2.h"
#include "KISS.h"
#include "constants.h"
#include "testing_support.h"
#include "ExternalWatchdog.h"
#include "efuse.h"
#include "radiohw.h"

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

// globals, basically things that need to be retained for each iteration of loop()

CircularBuffer<byte, CMDBUFFSIZE> cmdbuffer;
CircularBuffer<byte, DATABUFFSIZE> databuffer;
CircularBuffer<byte, TXBUFFSIZE> txbuffer;

int cmdpacketsize {0};  //really the size of the first packet in the buffer  Should think about whether or not these could be local vs. global
int datapacketsize {0};
int txbufflen {0}; //size of next packet in buffer

//radio config and interface
ax_packet rx_pkt;  //instance of packet structure

//radio config
ax_config config;

//modulation structure
ax_modulation modulation;

//two state variables
bool transmit {false};  // by default, we are not transmitting; might use the other bits in this for FIFO flags?
bool fault {false};

//timing 
//unsigned int lastlooptime {0};  //for timing the loop (debug)
unsigned int rxlooptimer {0};  //for determining the delay before switching modes (part of CCA)

Generic_LM75_10Bit tempsense(0x4B);

ExternalWatchdog watchdog(WDTICK);
Efuse efuse(Current_5V, OC5V, Reset_5V);
Radio radio(TX_RX, RX_TX, PAENABLE, SYSCLK, AX5043_DCLK, AX5043_DATA, PIN_LED_TX);
packet cmdpacket;
Command command;


void setup()
{  
  efuse.begin();
  radio.begin();
  
  pinMode(EN0, OUTPUT);          //enable serial port differential driver
  pinMode(EN1, OUTPUT);          //enable serial port differential driver
  pinMode(SELBAR, OUTPUT);       //select for the AX5043 SPI bus
  
  //pinMode(GPIO15, OUTPUT);       //test pin output
  //pinMode(GPIO16, OUTPUT);       //test pin output

  // start the I2C interface and the serial ports
  Wire.begin();

#ifdef _RADIO_BOARD_
  //query the temp sensor
  float patemp{tempsense.readTemperatureC()};
  Serial.print("temperature of PA: ");
  Serial.println(patemp);

  //enable the differential serial port drivers (Silversat board only)
  digitalWrite(EN0, HIGH);
  digitalWrite(EN1, HIGH);
#endif

  Serial.begin(115200);
  //while (!Serial) {};
  Serial1.begin(19200);  //I repeat...Serial 1 is Payload (RPi)
  //while (!Serial1) {};
  Serial0.begin(57600);  //I repeat...Serial 0 is Avionics  NOTE: this was slowed from 57600 for packet testing
  //while(!Serial0) {};  //taken out or we're waiting for a port we're not testing at the moment

  //serial port roll call
  Serial.println("I'm Debug");
  //Serial1.println("I'm Payload");
  //Serial0.println("I'm Avionics");

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

  //populate default modulation structure
  //fill the ax5043 config array with zeros
  memset(&modulation, 0, sizeof(ax_modulation));
  //modulation = gmsk_modulation;  //by default we're using gmsk, and allowing other MSK/FSK type modes to be configured by modifying the structure
  modulation.modulation = AX_MODULATION_FSK;
  modulation.encoding = AX_ENC_NRZI;
  modulation.framing = AX_FRAMING_MODE_HDLC | AX_FRAMING_CRCMODE_CRC_16;
  modulation.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
  modulation.bitrate = 9600;
  modulation.fec = 0;
  modulation.power = 1.0;
  modulation.continuous = 0;
  modulation.fixed_packet_length=0;
  modulation.parameters = {.fsk = { .modulation_index = 0.5 }};
  modulation.max_delta_carrier = 0;
  modulation.par = {};

  ax_init(&config);  //this does a reset, so needs to be first

  //load the RF parameters for the current config
  ax_default_params(&config, &modulation);  //ax_modes.c for RF parameters

  //parrot back what we set
  debug_printf("config variable values: \r\n");
  debug_printf("tcxo frequency: %d \r\n", int(config.f_xtal));
  debug_printf("synthesizer A frequency: %d \r\n", int(config.synthesiser.A.frequency));
  debug_printf("synthesizer B frequency: %d \r\n", int(config.synthesiser.B.frequency));
  debug_printf("status: %x \r\n", ax_hw_status());

  //turn on the receiver
  ax_rx_on(&config, &modulation);

  //for RF debugging
  // printRegisters(config);

  watchdog.begin();

  // for efuse testing; make sure to bump the watchdog
  //efuseTesting(efuse, watchdog);
}


void loop() 
{
  //noInterrupts();
  //debug_printf("%x \r\n", micros() - lastlooptime);
  //lastlooptime = micros();
  
  //interface handler - the interface handler processes packets coming in via the serial interfaces.
  if (Serial0.available() > 0) 
  {
    cmdbuffer.push(Serial0.read());  //we add data coming in to the tail...what's at the head is the oldest packet
    if (cmdbuffer.isFull()) 
    {
      debug_printf("ERROR: CMD BUFFER OVERFLOW \r\n");  //to date, have never seen this (or the data version) ever happen.
    }
  }

  //data, put it into its own buffer
  if (Serial1.available() > 0) 
  {
    databuffer.push(Serial1.read());  //we add data coming in to the tail...what's at the head is the oldest packet delimiter
    if (databuffer.isFull()) 
    {
      debug_printf("ERROR: DATA BUFFER OVERFLOW \r\n");
    }
  }

  //should be able to only to run processbuff after the first packet is removed.  But need to set some variable to
  //let it know that the buffer was removed.
  //process the command buffer first - processbuff returns the size of the first packet in the buffer, returns 0 if none
  cmdpacketsize = processbuff(cmdbuffer);
  //debug_printf("command packet size: %i \r\n", cmdpacketsize);

  //process the databuffer - see note above about changing the flow
  datapacketsize = processbuff(databuffer);
  //debug_printf("datapacketsize: %i \r\n", datapacketsize);

 //-------------end interface handler--------------

 //------------begin data processor----------------

  //only run this if there is a complete packet in the buffer, AND the data buffer is empty or the last byte in it is 0xC0...this is to sync writes from cmdbuffer into databuffer
  if (cmdpacketsize != 0 && (databuffer.isEmpty() || databuffer.last() == constants::FEND))  
  {
    debug_printf("command received, processing \r\n");
    //processcmdbuff(cmdbuffer, databuffer, cmdpacketsize, config, modulation, transmit, watchdog, efuse, radio, fault);
    //in the old command processor, processing the buffer and processing the command were done in the same function
    //they're now separated.  processcmdbuff looks at the command code, and if its for the other end, pushes it to the data buffer
    //otherwise it pulls the packet out of the buffer and sticks it into a cmdpacket structure.  
    //the command is then processed in processcommand.
    bool command_in_buffer = command.processcmdbuff(cmdbuffer, databuffer, cmdpacketsize, cmdpacket);
    //for commandcodes of 0x00 or 0xAA, it should just take the packet out of the command buffer and write it to the data buffer
    if (command_in_buffer)
    {
      debug_printf("command in main: %x \r\n", cmdpacket.commandcode);
      debug_printf("command buffer size: %i \r\n", cmdbuffer.size());
      command.processcommand(databuffer, cmdpacket, config, modulation, watchdog, efuse, radio, fault);
    }
  }

  //prepare a packet for transmit; the transmit loop will reset txbufflen to 0 after transmitting the buffer
  if (datapacketsize != 0)
  {  
    if (txbuffer.size() == 0) //just doing the next packet to keep from this process from blocking too much
    { 
      //mtu_size includes TCP/IP headers, but the 
      byte kisspacket[2*constants::mtu_size + 9];  //allow for a very big kiss packet, probably overkill (abs max is, now 512 x 2 + 9)  9 = 2 delimiters, 1 address, 4 TUN, 2 CRC
      byte nokisspacket[constants::mtu_size + 5]; //should be just the data plus, 5 = 1 address, 4 TUN
      //note this REMOVES the data from the databuffer...no going backsies
      for (int i = 0; i < datapacketsize; i++) 
      {
        kisspacket[i] = databuffer.shift();
      }
      txbufflen = kiss_unwrap(kisspacket, datapacketsize, nokisspacket); // kiss_unwrap returns the size of the new buffer
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
      radio.setReceive(config, modulation);  //this also changes the config parameter for the TX path to differential
      debug_printf("State changed to FULL_RX \r\n");
    }
    else if (ax_RADIOSTATE(&config) == 0)  //radio is idle, so we can transmit a packet, keep this non-blocking if it's active so we can process the next packet
    {    
      debug_printf("transmitting packet \r\n");
      //debug_printf("txbufflen: %x \r\n", txbufflen);
      byte txqueue[512];
      for (int i=0; i<txbufflen; i++)  //clear the transmitted packet out of the buffer and stick it in the txqueue
      //we had to do this because txbuffer is of type CircularBuffer, and ax_tx_packet is expecting a pointer.
      //might change this to store pointers in the circular buffer (see object handling in Circular Buffer reference)
      //and just create an array of stored packets
      //TODO: alternatively see if this compiles without recasting the txbuffer and passing it directly.
      {
        txqueue[i] = txbuffer.shift();
      }
      // digitalWrite(PIN_LED_TX, HIGH); 
      ax_tx_packet(&config, &modulation, txqueue, txbufflen);  //transmit the decoded buffer, this is blocking except for when the last chunk is committed.
      //this is because we're sitting and checking the FIFCOUNT register until there's enough room for the final chunk.
      //debug_printf("txbufflen (after transmit): %x \r\n", txbufflen);
      //debug_printf("txbuff size: %x \r\n", txbuffer.size());
      debug_printf("databufflen: %x \r\n", databuffer.size());
      debug_printf("cmdbufflen: %i \r\n", cmdbuffer.size());
      // digitalWrite(PIN_LED_TX, LOW);  
    }   
  }  
  //-------------end transmit handler--------------

  //receive handler
  if (transmit == false) 
  {
    if (ax_rx_packet(&config, &rx_pkt))  //the FIFO is not empty...there's something in the FIFO and we've just received it.  rx_pkt is an instance of the ax_packet structure
    {
      byte rxpacket[1026];  //this is the KISS encoded received packet, 2x max packet size plus 2...currently set for 512 byte packets, but this could be scaled if memory is an issue
      debug_printf("got a packet! \r\n");
      rxlooptimer = micros();
      //if it's HDLC, then the "address byte" (actually the KISS command byte) is in rx_pkt.data[0], because there's no length byte
      //by default we're sending out data, if it's 0xAA, then it's a command destined for the base/avoinics endpoint
       
      //So in this case we want the first byte (yes, we do) and we don't want the last 2 (for CRC-16..which hdlc has left for us)   
      int rxpacketlength { kiss_encapsulate(rx_pkt.data, rx_pkt.length-2, rxpacket) };  //remove the 2 extra bytes from the received packet length    
             
      if (rx_pkt.data[0] != 0xAA)  //packet.data is type byte
      {                    
        //there are only 2 endpoints, data (Serial1) or command responses (Serial0), rx_pkt is an instance of the ax_packet structure that includes the metadata
        Serial1.write(rxpacket, rxpacketlength);  //so it's data..send it to payload or to the proxy
      } 
      else 
      {        
        Serial0.write(rxpacket, rxpacketlength);  //so it's a command response , assumption is first byte of command or response is a zero..indicating that it goes to Avionics.  This can be replaced with something more complex
      }
      
    } 
    else { //the fifo is empty

      bool channelclear = assess_channel(rxlooptimer);

      if ((datapacketsize != 0) && channelclear == true) 
      {  
        //there's something in the tx buffers and the channel is clear
        printf("delay %lu \r\n", micros() - rxlooptimer);  //for debug to see what actual delay is
        rxlooptimer = micros();  //reset the receive loop timer to current micros()  
        radio.setTransmit(config, modulation);  //this also changes the config parameter for the TX path to single ended
        debug_printf("State changed to FULL_TX \r\n");
        transmit = true;
      }
    }
  }
  //-------------end receive handler--------------
  watchdog.trigger();  //I believe it's enough to just trigger the watchdog once per loop.  If it branches to commands, it's handled there.
  fault = efuse.overcurrent(transmit);
}
//-------------end loop--------------

bool assess_channel(int rxlooptimer) 
{
    //this is now a delay, not an averaging scheme.  Original implementation wasn't really averaging either because loop was resetting the first measurement
    //could retain the last one in a global and continually update it with the current average..but lets see if this works.
    if ((micros() - rxlooptimer) > constants::tx_delay)
    {
      int rssi = ax_RSSI(&config);  //now take a sample
      //avgrssi = (firstrssi + secondrssi)/2;  //and compute a new average
      if (rssi > constants::clear_threshold)
      {
        rxlooptimer = micros();
        return false;
        //printf("channel not clear");          
      }
      else 
      {
        return true;
        //printf("channel is clear");          
      }
    } 
    else 
    {
      return false;
      //timer hasn't expired
    }
  }

  //wiring_spi_transfer defines the chip selects on the SPI bus
  void wiring_spi_transfer(byte* data, uint8_t length) 
  {
    digitalWrite(SELBAR, LOW);   //select
    SPI.transfer(data, length);  //do the transfer
    digitalWrite(SELBAR, HIGH);  //deselect
  }