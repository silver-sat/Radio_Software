/**
 * @file e2e_commands_metro.ino
 * @author Tom Conrad (tom@silversat.org)
 * @brief end to end commands using Silversat radio board
 * @version 1.0.2
 * @date 2024-07-24
 *
 * I sooo want to change the name of this...but I fear something will break and it will take me forever to fix it...
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
 * debug output goes to Serial, the USB serial port on the SAMD21
 *
 * NOTE: there is no need to switch PA path.  It's handled internally, diff for RX, se for TX, but make sure you have the right #define in ax.cpp
 * ALSO NOTE: you must use HDLC to use FEC
 *
 * constants are defined in constants.cpp
 * some of these are a guess at the moment and probably way too large.
 * delay values are available for the time to wait after setting T/R lines and time to allow PA to stabilize
 * pa_delay is time to wait after switching on the PA
 * tx_delay is the delay before switching from RX to TX.  Once it switches, it sends a packet immediately.
 * clear_threshold is the threshold to declare the channel clear
 * mtu_size is the mtu size defined in tnc attach.  There are 4 additional bytes for the TUN interface header.  That is all transmitted, since it's within the KISS frame on Serial interface
 * The serial interface is KISS encoded.  When prepped for transmit the processor removes the KISS formatting, but retains the KISS command byte
 * The radio adds 2 additional CRC bytes (assuming we're using HDLC and on of the two 16 bit CRCs), except in reed-solomon mode or when using RAW framing
 * where it sends 32 checksum bytes (RS) or doesn't forward them (RAW).
 * When received by the remote radio, and after the data is handed to the processor, we remove the CRC bytes and reapply KISS encoding prior to writing to the appropriate port.

 * the main loop consists of three main parts; an interface handler, a transmit handler and a receive handler.
 * the loop needs to be tight and generally non-blocking because we are polling the radio (not using interrupts for now)

 * interface handler - the interface handler processes packets coming in via the serial interfaces.
 * for incoming data from the GS or avionics, just stick it into the circular buffer
 *
 * an ALTERNATIVE would be to identify a packet as it comes in (look for C0), and then decode it and store it into a Packet object
 * (inside the circular buffer, which would now hold packet objects).
 * You'd have to KISS decode on the fly, which might not be all that difficult (cmds are really easy, ASCII only, so there are no escape characters)
 * Payload data will contain escapes, so it has to be processed.
 * The advantage is that you don't need to process prior to transmit..just grab the next object in the buffer and ship it out.
 * It also might allow some direct processing of the packet data.  But remember that data is coming in fast (about 87 uS per byte @ 115.2k) leaving only so many
 * clock cycles to do the processing (about 3.8k cycles per byte), but this could be efficient, provided the radio keeps getting serviced as needed.
 * Maybe combine this with interrupts.
 *
 * Enable COMMANDS_ON_DEBUG_SERIAL to allow commands to be pushed into the command buffer from the debug serial port.  This is intended to support talking
 * to the system after it's buttoned up (via the SCIC port on the outside of the satellite)
 *
*/

#define _RADIO_BOARD_ // this is needed for variant file...see variant.h
// *******NOTE: you will need to modify the Serial buffer size in RingBuffer.h. ***************
// #define SERIAL_BUFFER_SIZE 1024  //this has been changed in RingBuffer.h  This is located in 1.7.16/cores/arduino.

//#define COMMANDS_ON_DEBUG_SERIAL


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__


#define CIRCULAR_BUFFER_INT_SAFE

// custom local files
#include "packetfinder.h"
#include "packet.h"
#include "commands.h"
#include "KISS.h"
#include "constants.h"
#include "testing_support.h"
#include "ExternalWatchdog.h"
#include "efuse.h"
#include "radio.h"
#include "stats.h"

// the AX library
#include "ax.h"

#include <CircularBuffer.hpp>
#include <SPI.h>
#include <Wire.h>
#include <FlashStorage.h>
#include <Temperature_LM75_Derived.h>
#include <ArduinoLog.h>
#include  "il2p.h"
#include "il2p_crc.h"
#include "FastCRC.h"
#include "Chrono.h"

#define CMDBUFFSIZE 512   // this buffer can be smaller because we control the rate at which packets come in
#define DATABUFFSIZE 8192 // how many packets do we need to buffer at most during a TCP session?
#define TXBUFFSIZE 512   // at most 4 256-byte packets, but if storing Packet class objects, need to figure out how big they are

// globals, basically things that need to be retained for each iteration of loop()

CircularBuffer<byte, CMDBUFFSIZE> cmdbuffer;
CircularBuffer<byte, DATABUFFSIZE> databuffer;
CircularBuffer<byte, TXBUFFSIZE> txbuffer;  //txbuffer should only hold decoded kiss packets (255 bytes max), probably packet class objects

Packet datapacket;

int cmdpacketsize{0}; // really the size of the first packet in the buffer  Should think about whether or not these could be local vs. global
int datapacketsize{0};
//int txbufflen{0}; // size of next packet in buffer

// three state variables
bool transmit{false}; // by default, we are not transmitting; might use the other bits in this for FIFO flags?
bool fault{false};
bool board_reset;   // Detects is the board was reset. It is set to true in void setup()

// timing
// unsigned int lastlooptime {0};  //for timing the loop (debug)
unsigned int rxlooptimer{0}; // for determining the delay before switching modes (part of CCA)

Generic_LM75_10Bit tempsense(0x4B);

ExternalWatchdog watchdog(WDTICK);
Efuse efuse(Current_5V, OC5V, Reset_5V);

Radio radio(TX_RX, RX_TX, PAENABLE, SYSCLK, AX5043_DCLK, AX5043_DATA, PIN_LED_TX, IRQ);
//DataPacket txpacket[8];  //these are not KISS encoded...unwrapped
Command command;

FlashStorage(operating_frequency, int);
FlashStorage(clear_threshold, byte);
byte clearthreshold{constants::clear_threshold};

volatile int reset_interrupt{0};

Chrono loop_timer(Chrono::MICROS);
unsigned long loop_time{0};

Chrono process_timer(Chrono::MICROS);
unsigned long processing_time{0};

Stats stats;


void setup()
{
    // startup the efuse
    efuse.begin();

    Serial.begin(57600);
    //while(!Serial);  //take this out later!

    //Log.begin(LOG_LEVEL_SILENT, &Serial, true);
    //Log.begin(LOG_LEVEL_ERROR, &Serial, true);
    //Log.begin(LOG_LEVEL_WARNING, &Serial, true);
    //Log.begin(LOG_LEVEL_TRACE, &Serial, true);
    Log.begin(LOG_LEVEL_NOTICE, &Serial, true);
    //Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);

    // Available levels are:
    // LOG_LEVEL_SILENT, LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARNING, LOG_LEVEL_INFO, LOG_LEVEL_TRACE, LOG_LEVEL_VERBOSE
    // Note: if you want to fully remove all logging code, uncomment #define DISABLE_LOGGING in Logging.h
    //       this will significantly reduce your project size

    //if (SERIAL_BUFFER_SIZE != 1024) Log.error(F("Serial buffer size is too small.  Modify RingBuffer.h \r\n"));

    // at first start the value stored in flash will be zero.  Need to update it to the default frequency and go from there
    if (operating_frequency.read() == 0)
    {
        operating_frequency.write(constants::frequency);
    }

    if (clear_threshold.read() == 0)
    {
        clear_threshold.write(clearthreshold);
    }

    clearthreshold = clear_threshold.read();

    // define spi select and serial port differential drivers
    pinMode(SELBAR, OUTPUT); // select for the AX5043 SPI bus
    pinMode(EN0, OUTPUT);    // enable serial port differential driver
    pinMode(EN1, OUTPUT);    // enable serial port differential driver
    pinMode(PIN_LED_TX, OUTPUT);
    // pinMode(GPIO15, OUTPUT);       //test pin output
    // pinMode(GPIO16, OUTPUT);       //test pin output

    // turn on the ports
    digitalWrite(EN0, true);
    digitalWrite(EN1, true);

    //reset indicator
    int state{0};
    Log.notice(F("**********BOARD RESET*********\r\n"));
    for (int i=0; i<10; i++)
        {
            digitalWrite(PIN_LED_TX, state);
            state = !state;
            delay(100);  //fast flashes to show we did a reset
        }

    // setup the watchdog
    watchdog.begin();

    // start SPI, configure and start up the radio
    Log.notice(F("starting up the radio\r\n"));
    SPI.begin();
    SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0)); // these settings seem to work, but not optimized

    radio.begin(wiring_spi_transfer, operating_frequency, clear_threshold);

    radio.printParamStruct();

    // start the I2C interface and the debug serial port
    Wire.begin();

#ifdef SILVERSAT
    // query the temp sensor
    float patemp = tempsense.readTemperatureC();
    Log.verbose(F("temperature of PA: %F\r\n"), patemp);
    

    // enable the differential serial port drivers (Silversat board only)
    digitalWrite(EN0, HIGH);
    digitalWrite(EN1, HIGH);
#endif

    // start the other serial ports
    Serial1.begin(9600); // I repeat...Serial 1 is Payload (RPi)
    Serial0.begin(19200); // I repeat...Serial 0 is Avionics  NOTE: this was slowed from 57600 for packet testing

    //attach the interrupt for the PAEnable pin
    attachInterrupt(digitalPinToInterrupt(IRQ), ISR, RISING);

    il2p_init(); //this has to be called to initialize the RS tables.

    // ** the following need to have testing_support.h included.
    // for efuse testing; make sure to bump the watchdog
    // efuseTesting(efuse, watchdog);
    // dump the registers and just hang...
    //printRegisters(radio);
    //il2p_testing();
    //while(1);

    // Detect if the board was reset
    // I don't think this line does what you think.  It defines a local version of board_reset with setup scope,  
    // versus setting the global variable.  did you mean to redefine it or just set it?  --tkc
    board_reset = true;
    
    loop_timer.restart();
}

void loop()
{
    loop_time = loop_timer.elapsed();
    if (loop_time > stats.max_loop_time) 
    {
        stats.max_loop_time = loop_time;
        //Log.notice("new max loop time: %lu \r\n", max_loop_time);
    }
    loop_timer.restart();

#ifdef COMMANDS_ON_DEBUG_SERIAL
    // debug interface can now be used to send raw commands in via the SCIC interface pins
    while (Serial.available() > 0)
    {
        cmdbuffer.push(Serial.read()); // we add data coming in to the tail...what's at the head is the oldest packet
        if (cmdbuffer.isFull())
        {
            Log.error(F("ERROR: CMD BUFFER OVERFLOW\r\n")); // to date, have never seen this (or the data version) ever happen.
        }
    }
#endif

    process_timer.restart();
    // interface handler - the interface handler processes packets coming in via the serial interfaces.
    int serial0_bytes = Serial0.available();
    if (serial0_bytes > stats.max_buffer_load_s0) stats.max_buffer_load_s0 = serial0_bytes;
    while (Serial0.available() > 0)
    {
        // Log.trace("%i\r\n", serial0_bytes);
        cmdbuffer.push(Serial0.read()); // we add data coming in to the tail...what's at the head is the oldest packet
        if (cmdbuffer.size() > stats.max_commandbuffer_load) stats.max_commandbuffer_load = cmdbuffer.size();
        if (cmdbuffer.isFull()) Log.error(F("ERROR: CMD BUFFER OVERFLOW\r\n")); 
    }

    int serial1_bytes = Serial1.available();
    if (serial1_bytes > 0) Log.trace("data in serial1 buffer\r\n");
    if (serial1_bytes > stats.max_buffer_load_s1) stats.max_buffer_load_s1 = serial1_bytes;  //tracking max buffer load

    // data, put it into its own buffer
    while (Serial1.available() > 0)
    {
        if (Serial1.available() > 350) Log.error(F("SERIAL BUFFER OVERFLOW"));
        databuffer.push(Serial1.read()); // we add data coming in to the tail...what's at the head is the oldest packet delimiter
        if (databuffer.size() > stats.max_databuffer_load) stats.max_databuffer_load = databuffer.size(); //tracking max buffer load
        if (databuffer.isFull()) Log.error(F("ERROR: DATA BUFFER OVERFLOW\r\n"));
    }

    // process the command buffer first - processbuff returns the size of the first packet in the buffer, returns 0 if none 
    cmdpacketsize = processbuff(cmdbuffer);

    //  error if buffer is growing out of bounds and no packet detected
    if (cmdpacketsize == 0 && cmdbuffer.size() > 255) Log.error(F("buffer processing error!!!"));

    unsigned long time_to_cmd = process_timer.elapsed();
    //Log.verbose("cmd buff processing time: %u \r\n", time_to_cmd);

    if (cmdpacketsize > 0) Log.verbose("command packet size: %i \r\n", cmdpacketsize);

    // process the databuffer - see note above about changing the flow
    datapacketsize = processbuff(databuffer);
    if (datapacketsize > 0) Log.verbose("datapacketsize: %i \r\n", datapacketsize);

    //  error if buffer is growing out of bounds and no packet detected
    if (datapacketsize == 0 && databuffer.size() > 255) Log.error(F("buffer processing error!!!"));  

    //measure max interface handler time
    processing_time = process_timer.elapsed();
    //Log.verbose("data buff processing time: %u\r\n", processing_time - time_to_cmd);
    if (processing_time > stats.max_interface_handler_execution_time) stats.max_interface_handler_execution_time = processing_time;

    //-------------end interface handler--------------

    //------------begin data processor----------------

    // only run this if there is a complete packet in the buffer, AND the data buffer is empty or the last byte in it is 0xC0...this is to sync writes from cmdbuffer into databuffer
    process_timer.restart();

    if (cmdpacketsize !=0)
    {
        Log.notice("databuffer.last: %X \r\n", databuffer.last());
        Log.notice("databuffer.isEmpty: %X \r\n", databuffer.isEmpty());
        Log.notice("cmdbuffer.size: %i\r\n", cmdbuffer.size());
        Log.notice("datapacketsize: %i\r\n", datapacketsize);
    }

    if ((cmdpacketsize != 0 && databuffer.isEmpty()) || (cmdpacketsize != 0 && (databuffer.last() == constants::FEND)))
    {
        Log.notice(F("command received, processing\r\n"));
        // processcmdbuff() looks at the command code, and if its for the other end, pushes it to the data buffer
        // otherwise it pulls the packet out of the buffer and sticks it into a cmdpacket structure. (that allows for more complex parsing if needed/wanted)
        Packet cmdpacket;
        cmdpacket.packetlength = cmdpacketsize;

        int freemem = freeMemory();
        if (freemem < stats.free_mem_minimum) stats.free_mem_minimum = freemem;

        bool command_in_buffer = cmdpacket.processcmdbuff(cmdbuffer, databuffer);
        // for commandcodes of 0x00 or 0xAA, it takes the packet out of the command buffer and writes it to the data buffer
        if (command_in_buffer) command.processcommand(databuffer, cmdpacket, watchdog, efuse, radio, fault, 
            operating_frequency, clear_threshold, clearthreshold, board_reset, stats);
        // once the command has been completed the Packet instance goes out of scope and is deleted
    }

    // prepare a packet for transmit
    if (datapacketsize != 0)
    {
        // Log.notice("there's something in the data buffer (datapacketsize !=0)\r\n");
        uint32_t ax25_tx_crc_encoded;

        if (txbuffer.size() == 0) // just doing the next packet to keep from this process from blocking too much
        {
            Log.trace(F("txbuffer size is zero\r\n"));
            // we need to keep the complete KISS packet together in order to unwrap it, but we can pull the command code out
            byte kisspacket[512]; //prepare to pull the packet out of the buffer...I re-opted for a fixed array size, so it did not have to get dynamically resized.
            unsigned char parity_data[16];
            unsigned char parity_header[2];
            unsigned char il2p_header_scrambled[13];
            unsigned char il2p_data[constants::max_packet_size];
            
            // REMOVE the data from the databuffer...no going backsies
            for (int i = 0; i < datapacketsize; i++) kisspacket[i] = databuffer.shift();

            datapacket.packetlength = kiss_unwrap(kisspacket, datapacketsize, datapacket.packetbody); // kiss_unwrap returns the size of the new buffer and creates the decoded packet
            Log.trace(F("unwrapped packet size: %i \r\n"), datapacket.packetlength);
            datapacket.commandcode = datapacket.packetbody[0];
            /*
            for (int i=0; i<datapacket.packetlength; i++) Log.trace("%X", datapacket.packetbody[i]);
            Log.trace("\r\n");
            */

            //okay, now that we have the decoded packet, we need to compute the il2p header (assuming that il2p is turned on) and prepend that to the data
            //then we need to RS encode that (using the il2p encoder, so again, only if il2p is enabled)

            if (radio.modulation.il2p_enabled == 1)
            {
                int il2p_payload_length = datapacket.packetlength - 1;  //this is just for readability
                //compute the il2p header
                Log.trace(F("construct IL2P packet\r\n"));
                unsigned char il2p_header_precoded[13]{0x6B, 0xE3, 0x41, 0x76, 0x76, 0x37, 0x2B, 0x23, 0x01, 0x36, 0x76, 0x77, 0x10};
                
                //we're not including the command byte in the payload, but it's in datapacket.packetbody, so there's bunch of +/-1's here and there
                for (int i=2; i<12; i++) il2p_header_precoded[i] |= ((il2p_payload_length >> (11-i)) & 0x01) << 7;

                //scramble it
                Log.trace(F("scrambling header\r\n"));
                il2p_scramble_block(il2p_header_precoded, il2p_header_scrambled, 13);
                Log.verbose(F("scrambled header: \r\n"));
                for (int i = 0; i < 13; i++) Log.verbose("%X, ", il2p_header_scrambled[i]);
                Log.verbose("\r\n");

                //now encode it
                Log.trace(F("encoding header\r\n"));
                //il2p_encode_rs(il2p_header_scrambled, header_size, parity_size, parity);
                il2p_encode_rs(il2p_header_scrambled, 13, 2, parity_header);
                Log.verbose(F("parity:\r\n"));
                for (int i = 0; i < 2; i++)  Log.verbose("%X, ", parity_header[i]);   
                
                //now we get to add the data...yay!
                //scramble the block
                Log.trace(F("scrambling data\r\n"));
                il2p_scramble_block(datapacket.packetbody+1, il2p_data, il2p_payload_length); //taking out the command code byte
                IL2P_CRC il2p_crc;
                uint16_t ax25_tx_crc = il2p_crc.calculate_AX25(datapacket.packetbody+1, il2p_payload_length);
                Log.notice("AX25 CRC (TX) = %X\r\n", ax25_tx_crc);
                ax25_tx_crc_encoded = il2p_crc.encode_crc(ax25_tx_crc);

                //now encode that
                Log.trace(F("encoding data\r\n"));
                il2p_encode_rs(il2p_data, il2p_payload_length, 16, parity_data);
                datapacket.packetlength += 31; //16 parity bytes + 15 header bytes
            }

            if (radio.modulation.il2p_enabled == 1)
            {   
                Log.trace(F("initial txbuffer size: %i\r\n"), txbuffer.size());
                unsigned char il2p_framing[3]{0xF1, 0x5E, 0x48};
                
                //re-add the command byte
                Log.trace(F("adding the command byte\r\n"));
                txbuffer.push(datapacket.commandcode);

                //first the framing bytes
                Log.trace(F("adding on the IL2P framing bytes\r\n"));
                for (int i=0; i< 3; i++) txbuffer.push(il2p_framing[i]);
                datapacket.packetlength += 3; //add the three bytes to the total
                
                //next the header
                Log.trace(F("pushing the IL2P header\r\n"));
                for (int i = 0; i < 13; i++) txbuffer.push(il2p_header_scrambled[i]);

                //next the header parity
                Log.trace(F("pushing the IL2P header parity\r\n"));
                for (int i = 0; i < 2; i++) txbuffer.push(parity_header[i]);

                //next the data (scrambled)
                Log.trace(F("pushing the IL2P data\r\n"));
                for (int i = 0; i < (datapacket.packetlength-1)-3-31; i++) txbuffer.push(il2p_data[i]); //one less because of the command byte, three less for the framing

                //and add the parity next
                Log.trace(F("pushing the IL2P data parity\r\n"));
                for (int i = 0; i < 16; i++)txbuffer.push(parity_data[i]);
                
                //here is where we can add the CRC
                uint8_t crc_buffer[255];  //again, avoiding dynamic sizing
                txbuffer.copyToArray(crc_buffer);
                IL2P_CRC il2p_crc;
                Log.verbose(F("first buffer byte: %X\r\n"), *(crc_buffer+4));  //don't include the cmd and framing bytes = 4
                Log.verbose(F("last buffer byte: %X\r\n"), *(crc_buffer+txbuffer.size()-1));
                
                Log.trace(F("pushing the IL2P CRC\r\n"));
                Log.trace(F("Tx CRC: %X\r\n"), ax25_tx_crc_encoded);
                txbuffer.push((uint8_t)((ax25_tx_crc_encoded & 0xFF000000)>>24));
                txbuffer.push((uint8_t)((ax25_tx_crc_encoded & 0x00FF0000)>>16));
                txbuffer.push((uint8_t)((ax25_tx_crc_encoded & 0x0000FF00)>>8));
                txbuffer.push((uint8_t)(ax25_tx_crc_encoded & 0x000000FF));
                datapacket.packetlength += 4;
                //Log.verbose(F("Buffered Packet \r\n"));
                //for (int i=0; i<txbuffer.size(); i++) Log.verbose(F("Index: %d  Data: %X \r\n"), i, txbuffer[i]);
            }
            else
            {
                // push the unwrapped packet onto the tx buffer
                Log.notice(F("pushing packet into txbuffer\r\n"));
                for (int i = 0; i < datapacket.packetlength; i++) txbuffer.push(datapacket.packetbody[i]);
            }   
        }
    }

    processing_time = process_timer.elapsed();
    if (processing_time > stats.max_data_processor_execution_time) stats.max_data_processor_execution_time = processing_time;
    //-------------end data processor---------------------

    // transmit handler - the transmit handler processes data in the buffers when the radio is in transmit mode
    process_timer.restart();
    if (transmit == true)
    {
        if (reset_interrupt == 1)
        {
            digitalWrite(PAENABLE, LOW); // turn off the PA
            digitalWrite(PIN_LED_TX, LOW);
            //read the register to clear the interrupt
            ax_hw_read_register_16(&radio.config, AX_REG_RADIOEVENTREQ);
            Log.verbose("clearing the interrupt\r\n");
            reset_interrupt = 0;
        }
        //Log.notice(F("radio busy?: %X\r\n"), radio.radioBusy());
        if (datapacketsize == 0 && txbuffer.size() == 0) // datapacketsize should still be nonzero until the buffer is processed again (next loop)
        {
            //radio busy will only show idle as long as it's in FULLTX
            transmit = false; // change state and we should drop out of loop
            Log.notice("current power state: %X\r\n", radio.get_power_state());
            while (radio.radioBusy());

            radio.setReceive(); 
            Log.notice(F("State changed to FULL_RX\r\n"));
        }
        else if (!radio.radioBusy()) // radio is idle, so we can transmit a packet, keep this non-blocking if it's active so we can process the next packet
        {
            Log.notice(F("transmitting packet\r\n"));
            Log.verbose(F("datapacket.packetlength: %i\r\n"), datapacket.packetlength);
            Log.verbose(F("txbuffer.size: %i\r\n"), txbuffer.size());
            byte txqueue[512];  //allowing for future larger packets

            // clear the transmitted packet out of the buffer and stick it in the txqueue
            // we had to do this because txbuffer is of type CircularBuffer, and ax_tx_packet is expecting a pointer.
            // might change this to store pointers in the circular buffer (see object handling in Circular Buffer reference)
            // and just create an array of stored packets
            // TODO: alternatively see if this compiles without recasting the txbuffer and passing it directly.
            //txbuffer.copyToArray(txqueue);  //can't do this, it doesn't empty the buffer...but maybe just clear it?
            for (int i = 0; i < datapacket.packetlength; i++) txqueue[i] = txbuffer.shift();
            // transmit the decoded buffer, this is blocking except for when the last chunk is committed.
            // this is because we're sitting and checking the FIFOCOUNT register until there's enough room for the final chunk.
            radio.transmit(txqueue, datapacket.packetlength);

            Log.verbose(F("databufflen (post transmit): %i\r\n"), databuffer.size());
            Log.verbose(F("cmdbufflen (post transmit): %i\r\n"), cmdbuffer.size());
            Log.verbose(F("datapacket.packetlength (post transmit): %i\r\n"), txbuffer.size());

            if (databuffer.size() > 4096)
            {
                Log.warning(F("DATABUFFER at half full\r\n"));
                Log.warning(F("buffer size: %d\r\n"), databuffer.size());
            }
            if (stats.max_buffer_load_s0 > 350)  Log.warning(F("serial0 buffer overflow\r\n"));
            if (stats.max_buffer_load_s1 > 350)  Log.warning(F("serial1 buffer overflow\r\n"));
            Log.trace(F("freememory: %d\r\n"),freeMemory());
        }
    }
    processing_time = process_timer.elapsed();
    if (processing_time > stats.max_transmit_handler_execution_time) stats.max_transmit_handler_execution_time = processing_time;
    //-------------end transmit handler--------------

    // receive handler
    process_timer.restart();
    if (transmit == false)
    {
        if (radio.receive())
        {
            // rxpacket is the KISS encoded packet, 2x max packet size plus 2 C0
            // currently set for 256 byte packets, but this could be scaled if memory is an issue, who's going to send 256 escape characters?
            byte rxpacket[514];
            Log.trace(F("got a packet!\r\n"));
            Log.trace(F("packet length: %i\r\n"), radio.rx_pkt.length); // it looks like the two crc bytes are still being sent (or it's assumed they're there?)
            Log.trace(F("freememory: %d\r\n"),freeMemory());
            rxlooptimer = micros();
            int rxpacketlength{0};
            // if it's HDLC, then the "address byte" (actually the KISS command byte) is in rx_pkt.data[0], because there's no length byte
            // otherwise it's in rx_pkt.data.  Also HDLC adds the 2 crc bytes, but raw format doesn't have them.  RAW format adds a length byte
            // by default we're sending out data (cmd byte 0x00), if it's 0xAA, then it's a command destined for the base/avoinics endpoint

            //for (int i=0; i<radio.rx_pkt.length;i++) Log.verbose(F("rx data: %d, %X\r\n"), i, radio.rx_pkt.data[i]);

            // So in this case we want the first byte (yes, we do) and we don't want the last 2 (for CRC-16..which hdlc has left for us)
            int command_offset = 1;
            if ((radio.modulation.framing & 0xE) == AX_FRAMING_MODE_HDLC || (radio.modulation.il2p_enabled)) command_offset=0;
            //for il2p, i removed the length byte in ax.cpp

            if (radio.modulation.il2p_enabled)
            {
                rxpacketlength = kiss_encapsulate(radio.rx_pkt.data+command_offset, radio.rx_pkt.length, rxpacket);
            }
            else
            {
                rxpacketlength = kiss_encapsulate(radio.rx_pkt.data+command_offset, radio.rx_pkt.length-2+command_offset, rxpacket); // remove the 2 extra bytes from the received packet length
            }
            Log.trace(F("kiss packet length: %d\r\n"),rxpacketlength);
            Log.trace(F("command byte: %X\r\n"), radio.rx_pkt.data[command_offset]);

            if (radio.rx_pkt.data[command_offset] != 0xAA) // packet.data is type byte
            {
                // there are only 2 endpoints, data (Serial1) or command responses (Serial0), rx_pkt is an instance of the ax_packet structure that includes the metadata
                Serial1.write(rxpacket, rxpacketlength); // so it's data..send it to payload or to the proxy
            }
            else
            {
                // so it's a command response , assumption is first byte of command or response is 0xAA..indicating that it goes to Avionics.
                Serial0.write(rxpacket, rxpacketlength);
                // duplicate it on Serial
#ifdef COMMANDS_ON_DEBUG_SERIAL
                Serial.write(rxpacket, rxpacketlength);
#endif
            }
        }
        else
        { // the fifo is empty
            bool channelclear = radio.assess_channel(rxlooptimer);
            //Log.trace("channel clear?: %d\r\n", channelclear);
            
            if ((datapacketsize != 0) && channelclear == true )  //when receiving the radio state bounces between 0x0C and 0x0E until it actually starts receiving 0x0F
            //if ((datapacketsize != 0) && channelclear == true )
            {
                // there's something in the tx buffers and the channel is clear
                Log.notice(F("delay %lu\r\n"), micros() - rxlooptimer); // for debug to see what actual delay is
                rxlooptimer = micros();                                 // reset the receive loop timer to current micros()
                radio.setTransmit();                  // this also changes the radio.config parameter for the TX path to single ended
                Log.notice (F("State changed to FULL_TX\r\n"));
                transmit = true;
            }
        }
    }

    //measure max receive handler execution time
    processing_time = process_timer.elapsed();
    if (processing_time > stats.max_receive_handler_execution_time) stats.max_receive_handler_execution_time = processing_time;

    //-------------end receive handler--------------
    watchdog.trigger(); // I believe it's enough to just trigger the watchdog once per loop.  If it branches to commands, it's handled there.
    fault = efuse.overcurrent(transmit);
}

//-------------end loop--------------


// wiring_spi_transfer defines the chip selects on the SPI bus
void wiring_spi_transfer(byte *data, uint8_t length)
{
    digitalWrite(SELBAR, LOW);  // select
    SPI.transfer(data, length); // do the transfer
    digitalWrite(SELBAR, HIGH); // deselect
}

int freeMemory() 
{
  char top;
  return &top - reinterpret_cast<char*>(sbrk(0));
}

void ISR()
{
//we got an interrupt, so turn off the PA
digitalWrite(PAENABLE, LOW); // turn off the PA
digitalWrite(PIN_LED_TX, LOW);
reset_interrupt = 1;
}