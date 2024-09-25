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
 * The radio adds 2 additional CRC bytes (assuming we're using CRC-16), except in reed-solomon mode, where it sends 32 checksum bytes.
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

#define DEBUG

#define _RADIO_BOARD_ // this is needed for variant file...see variant.h
// #define SERIAL_BUFFER_SIZE 1024  //this is fixed in RingBuffer.h  This is located in 1.7.16/cores/arduino
#define COMMANDS_ON_DEBUG_SERIAL

/*
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
*/

#define CIRCULAR_BUFFER_INT_SAFE

// custom local files
#include "packetfinder.h"
#include "commands.h"
#include "KISS.h"
#include "constants.h"
#include "testing_support.h"
#include "ExternalWatchdog.h"
#include "efuse.h"
#include "radio.h"
#include "fec.h"

// the AX library
#include "ax.h"

#include <LibPrintf.h>
#include <CircularBuffer.hpp>
#include <SPI.h>
#include <Wire.h>
#include <FlashStorage.h>
#include <Temperature_LM75_Derived.h>

#include <vector>

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#define CMDBUFFSIZE 512   // this buffer can be smaller because we control the rate at which packets come in
#define DATABUFFSIZE 8192 // how many packets do we need to buffer at most during a TCP session?
#define TXBUFFSIZE 1024   // at most 2 packets

// globals, basically things that need to be retained for each iteration of loop()

CircularBuffer<byte, CMDBUFFSIZE> cmdbuffer;
CircularBuffer<byte, DATABUFFSIZE> databuffer;
CircularBuffer<byte, TXBUFFSIZE> txbuffer;

int cmdpacketsize{0}; // really the size of the first packet in the buffer  Should think about whether or not these could be local vs. global
int datapacketsize{0};
int txbufflen{0}; // size of next packet in buffer

// two state variables
bool transmit{false}; // by default, we are not transmitting; might use the other bits in this for FIFO flags?
bool fault{false};

// timing
// unsigned int lastlooptime {0};  //for timing the loop (debug)
unsigned int rxlooptimer{0}; // for determining the delay before switching modes (part of CCA)

Generic_LM75_10Bit tempsense(0x4B);

ExternalWatchdog watchdog(WDTICK);
Efuse efuse(Current_5V, OC5V, Reset_5V);

Radio radio(TX_RX, RX_TX, PAENABLE, SYSCLK, AX5043_DCLK, AX5043_DATA, PIN_LED_TX);
Packet cmdpacket;
Command command;

// debug variable
int max_buffer_load_s0 = 0;
int max_buffer_load_s1 = 0;

/*
//I'm expanding the macro so I know what class to pass...might be able to collapse this back.
//asks the compiler to create a 256 byte aligned variable '_datadefaultfrequency'
__attribute__((__aligned__(256))) static const uint8_t PPCAT(_data, default_frequency)[(sizeof(int) + 255) / 256 * 256] = {};
//and then uses that to define the location of default_frequency
FlashStorageClass<int> default_frequency(PPCAT(_data, default_frequency));
//so the variable to pass is default_frequency
*/

FlashStorage(operating_frequency, int);

std::vector<int> empty{};

void setup()
{
    // startup the efuse
    efuse.begin();

    // at start the value will be zero.  Need to update it to the default frequency and go from there
    if (operating_frequency.read() == 0)
    {
        operating_frequency.write(constants::frequency);
    }

    // define spi select and serial port differential drivers
    pinMode(SELBAR, OUTPUT); // select for the AX5043 SPI bus
    pinMode(EN0, OUTPUT);    // enable serial port differential driver
    pinMode(EN1, OUTPUT);    // enable serial port differential driver
    // pinMode(GPIO15, OUTPUT);       //test pin output
    // pinMode(GPIO16, OUTPUT);       //test pin output

    // turn on the ports
    digitalWrite(EN0, true);
    digitalWrite(EN1, true);

    // setup the watchdog
    watchdog.begin();

    // start SPI, configure and start up the radio
    debug_printf("starting up the radio\n");
    SPI.begin();
    SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0)); // these settings seem to work, but not optimized

    radio.begin(wiring_spi_transfer, operating_frequency);

    // start the I2C interface and the debug serial port
    Wire.begin();
    Serial.begin(115200);
    // while(!Serial);

#ifdef SILVERSAT
    // query the temp sensor
    float patemp{tempsense.readTemperatureC()};
    Serial.print("temperature of PA: ");
    Serial.println(patemp);

    // enable the differential serial port drivers (Silversat board only)
    digitalWrite(EN0, HIGH);
    digitalWrite(EN1, HIGH);
#endif

    // start the other serial ports
    Serial1.begin(19200); // I repeat...Serial 1 is Payload (RPi)
    Serial0.begin(19200); // I repeat...Serial 0 is Avionics  NOTE: this was slowed from 57600 for packet testing

    // for efuse testing; make sure to bump the watchdog
    // efuseTesting(efuse, watchdog);

    // dump the registers and just hang...
    // printRegisters(radio.config);
    // while(1);
}

void loop()
{
#ifdef COMMANDS_ON_DEBUG_SERIAL
    // debug interface can now be used to send raw commands in via the SCIC interface pins
    while (Serial.available() > 0)
    {
        cmdbuffer.push(Serial.read()); // we add data coming in to the tail...what's at the head is the oldest packet
        if (cmdbuffer.isFull())
        {
            debug_printf("ERROR: CMD BUFFER OVERFLOW \r\n"); // to date, have never seen this (or the data version) ever happen.
        }
    }
#endif

    // interface handler - the interface handler processes packets coming in via the serial interfaces.
    int serial0_bytes = Serial0.available();
    if (serial0_bytes > max_buffer_load_s0)
        max_buffer_load_s0 = serial0_bytes;

    while (Serial0.available() > 0)
    {
        // printf("%i \r\n", serial0_bytes);
        cmdbuffer.push(Serial0.read()); // we add data coming in to the tail...what's at the head is the oldest packet
        if (cmdbuffer.isFull())
        {
            debug_printf("ERROR: CMD BUFFER OVERFLOW \r\n"); // to date, have never seen this (or the data version) ever happen.
        }
    }

    int serial1_bytes = Serial1.available();
    if (serial1_bytes > max_buffer_load_s1)
        max_buffer_load_s1 = serial1_bytes;

    // data, put it into its own buffer
    while (Serial1.available() > 0)
    {
        databuffer.push(Serial1.read()); // we add data coming in to the tail...what's at the head is the oldest packet delimiter
        if (databuffer.isFull())
        {
            debug_printf("ERROR: DATA BUFFER OVERFLOW \r\n");
        }
    }

    // process the command buffer first - processbuff returns the size of the first packet in the buffer, returns 0 if none
    cmdpacketsize = processbuff(cmdbuffer);
    debug_printf("command packet size: %i \r\n", cmdpacketsize);

    // process the databuffer - see note above about changing the flow
    datapacketsize = processbuff(databuffer);
    debug_printf("datapacketsize: %i \r\n", datapacketsize);

    //-------------end interface handler--------------

    //------------begin data processor----------------

    // only run this if there is a complete packet in the buffer, AND the data buffer is empty or the last byte in it is 0xC0...this is to sync writes from cmdbuffer into databuffer
    if (cmdpacketsize != 0 && (databuffer.isEmpty() || databuffer.last() == constants::FEND))
    {
        debug_printf("command received, processing \r\n");
        // processcmdbuff() looks at the command code, and if its for the other end, pushes it to the data buffer
        // otherwise it pulls the packet out of the buffer and sticks it into a cmdpacket structure. (that allows for more complex parsing if needed/wanted)
        // the command is then processed using processcommand().
        bool command_in_buffer = command.processcmdbuff(cmdbuffer, databuffer, cmdpacketsize, cmdpacket);
        // for commandcodes of 0x00 or 0xAA, it takes the packet out of the command buffer and writes it to the data buffer
        if (command_in_buffer)
        {
            debug_printf("command in main: %x \r\n", cmdpacket.commandcode);
            debug_printf("command buffer size: %i \r\n", cmdbuffer.size());
            command.processcommand(databuffer, cmdpacket, watchdog, efuse, radio, fault, operating_frequency);
        }
    }

    // prepare a packet for transmit
    if (datapacketsize != 0)
    {
        if (txbuffer.size() == 0) // just doing the next packet to keep from this process from blocking too much
        {
            // mtu_size includes TCP/IP headers.  see next line.
            byte kisspacket[2 * constants::mtu_size + 9]; // allow for a very big kiss packet, probably overkill (abs max is, now 512 x 2 + 9)  9 = 2 delimiters, 1 address, 4 TUN, 2 CRC
            byte nokisspacket[constants::mtu_size + 37];  // should be just the data plus, 5 = 1 address, 4 TUN.  Now adding 32 extra bytes for reed solomon bits
            // byte rs_encoded_packet[constants::mtu_size + 37];
            for (int i = 0; i < datapacketsize; i++) // note this REMOVES the data from the databuffer...no going backsies
            {
                kisspacket[i] = databuffer.shift();
            }

            txbufflen = kiss_unwrap(kisspacket, datapacketsize, nokisspacket); // kiss_unwrap returns the size of the new buffer and creates the decoded packet

            /*
            for (int i=0; i<txbufflen; i++) debug_printf("%x", nokisspacket[i]);
            debug_printf("\r\n");
            */

            // only add the parity bytes if RS is enabled
            if (radio.modulation.rs_enabled == 1)
            {
                byte paritydata[32];
                rs_encode(paritydata, nokisspacket, txbufflen); // paritydata is the rs parity bytes, nokisspacket is the decoded kiss data and txbufflen is the size of the decoded data
                for (int i = txbufflen; i < (txbufflen + 32); i++)
                {
                    nokisspacket[i] = paritydata[i - txbufflen];
                }
                txbufflen += 32; // increase the length to transfer by the extra 32 bytes
                /*
                for (int i=0; i<txbufflen; i++) debug_printf("%x", nokisspacket[i]);
                debug_printf("\r\n");
                */
            }

            for (int i = 0; i < txbufflen; i++)
            {
                txbuffer.push(nokisspacket[i]); // push the unwrapped packet onto the tx buffer
            }
        }
    }
    //-------------end data processor---------------------

    // transmit handler - the transmit handler processes data in the buffers when the radio is in transmit mode
    if (transmit == true)
    {
        if (datapacketsize == 0 && txbuffer.size() == 0) // datapacketsize should still be nonzero until the buffer is processed again (next loop)
        {
            transmit = false; // change state and we should drop out of loop
            while (radio.radioBusy())
            {
            };                                    // check to make sure all outgoing packets are done transmitting
            radio.setReceive(); // this also changes the config parameter for the TX path to differential
            debug_printf("State changed to FULL_RX \r\n");
        }
        else if (!radio.radioBusy()) // radio is idle, so we can transmit a packet, keep this non-blocking if it's active so we can process the next packet
        {
            debug_printf("transmitting packet \r\n");
            // debug_printf("txbufflen: %x \r\n", txbufflen);
            byte txqueue[512];

            // clear the transmitted packet out of the buffer and stick it in the txqueue
            // we had to do this because txbuffer is of type CircularBuffer, and ax_tx_packet is expecting a pointer.
            // might change this to store pointers in the circular buffer (see object handling in Circular Buffer reference)
            // and just create an array of stored packets
            // TODO: alternatively see if this compiles without recasting the txbuffer and passing it directly.
            for (int i = 0; i < txbufflen; i++)
            {
                txqueue[i] = txbuffer.shift();
            }

            // transmit the decoded buffer, this is blocking except for when the last chunk is committed.
            // this is because we're sitting and checking the FIFOCOUNT register until there's enough room for the final chunk.
            radio.transmit(txqueue, txbufflen);

            // debug_printf("databufflen: %x \r\n", databuffer.size());
            // debug_printf("cmdbufflen: %i \r\n", cmdbuffer.size());
            // debug_printf("txbufflen: %i \r\n", txbuffer.size());
            // printf("max S0 tx buffer load: %i \r\n", max_buffer_load_s0);
            // printf("max S1 tx buffer load: %i \r\n", max_buffer_load_s1);
        }
    }
    //-------------end transmit handler--------------

    // receive handler
    if (transmit == false)
    {
        // the FIFO is not empty...there's something in the FIFO and we've just received it.  rx_pkt is an instance of the ax_packet structure
        if (radio.receive())
        {
            // rxpacket is the KISS encoded received packet, 2x max packet size plus 2...currently set for 512 byte packets, but this could be scaled if memory is an issue
            byte rxpacket[1026];
            debug_printf("got a packet! \r\n");
            debug_printf("packet length: %i \r\n", radio.rx_pkt.length); // it looks like the two crc bytes are still being sent (or it's assumed they're there?)
            rxlooptimer = micros();
            int rxpacketlength{0};
            // if it's HDLC, then the "address byte" (actually the KISS command byte) is in rx_pkt.data[0], because there's no length byte
            // by default we're sending out data, if it's 0xAA, then it's a command destined for the base/avoinics endpoint

            // So in this case we want the first byte (yes, we do) and we don't want the last 2 (for CRC-16..which hdlc has left for us)
            if (radio.modulation.rs_enabled)
            {
                // rs packets do not have the 2 CRC bytes and all the RS bytes were removed before writing to rx_pkt
                rxpacketlength = kiss_encapsulate(radio.rx_pkt.data, radio.rx_pkt.length, rxpacket); // remove the 2 extra bytes from the received packet length
            }
            else
            {
                rxpacketlength = kiss_encapsulate(radio.rx_pkt.data, radio.rx_pkt.length - 2, rxpacket); // remove the 2 extra bytes from the received packet length
            }

            if (radio.rx_pkt.data[0] != 0xAA) // packet.data is type byte
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
            // printf("max S0 tx buffer load: %i \r\n", max_buffer_load_s0);
            // printf("max S1 tx buffer load: %i \r\n", max_buffer_load_s1);
        }
        else
        { // the fifo is empty
            bool channelclear = assess_channel(rxlooptimer);
            if ((datapacketsize != 0) && channelclear == true)
            {
                // there's something in the tx buffers and the channel is clear
                debug_printf("delay %lu \r\n", micros() - rxlooptimer); // for debug to see what actual delay is
                rxlooptimer = micros();                                 // reset the receive loop timer to current micros()
                radio.setTransmit();                  // this also changes the radio.config parameter for the TX path to single ended
                debug_printf("State changed to FULL_TX \r\n");
                transmit = true;
            }
        }
    }
    //-------------end receive handler--------------
    watchdog.trigger(); // I believe it's enough to just trigger the watchdog once per loop.  If it branches to commands, it's handled there.
    fault = efuse.overcurrent(transmit);
}

//-------------end loop--------------

bool assess_channel(int rxlooptimer)
{
    // this is now a delay, not an averaging scheme.  Original implementation wasn't really averaging either because loop was resetting the first measurement
    // could retain the last one in a global and continually update it with the current average..but lets see if this works.
    if ((micros() - rxlooptimer) > constants::tx_delay)
    {
        int rssi = radio.rssi(); // now take a sample
        // avgrssi = (firstrssi + secondrssi)/2;  //and compute a new average
        if (rssi > constants::clear_threshold)
        {
            rxlooptimer = micros();
            return false;
            // printf("channel not clear");
        }
        else
        {
            return true;
            // printf("channel is clear");
        }
    }
    else
    {
        return false;
        // timer hasn't expired
    }
}

// wiring_spi_transfer defines the chip selects on the SPI bus
void wiring_spi_transfer(byte *data, uint8_t length)
{
    digitalWrite(SELBAR, LOW);  // select
    SPI.transfer(data, length); // do the transfer
    digitalWrite(SELBAR, HIGH); // deselect
}