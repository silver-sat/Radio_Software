// [Modified from GitHub josefmtd/kiss-aprs/KISS.h
// https://github.com/josefmtd/kiss-aprs/blob/88945ebbd482d26bc564d0dac64841bf0b622dbb/KISS.h
// by isaac-silversat of SilverSat Limited (https://www.silversat.org)]

#ifndef KISS_H
#define KISS_H

#include <Arduino.h>
#include <CircularBuffer.h> // Added by SilverSat

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#define CMD_DATA 0x00
#define CMD_TXDELAY 0x01
#define CMD_PERSIST 0x02
#define CMD_SLOTTIME 0x03
#define CMD_TXTAIL 0x04
#define CMD_DUPLEX 0x05
#define CMD_HARDWARE 0x06
#define CMD_RETURN 0xFF

// [KISS class commented]
// class KISSClass : public Stream {
//   public:
//     KISSClass(HardwareSerial& hwSerial);

//     virtual void begin(unsigned long baudRate);
//     virtual void begin(unsigned long baudRate, uint16_t config);
//     virtual void end();
//     virtual int available();
//     virtual int peek();
//     virtual int read(void);
//     virtual void flush();
//     virtual size_t write(uint8_t byte);
//     using Print::write;

//     void sendData(const char *buffer, int bufferSize);
//     void sendTxDelay(const uint8_t txDelay);
//     void sendPersist(const uint8_t persist);
//     void sendTimeSlot(const uint8_t timeSlot);
//     void sendTxTail(const uint8_t txTail);
//     void sendDuplex(const uint8_t duplex);

//   private:
//     HardwareSerial* _serial;
//     unsigned long _baudRate;
//     uint16_t _config;

//     uint8_t _txDelay;
//     uint8_t _persist;
//     uint8_t _timeSlot;
//     uint8_t _txTail;
//     uint8_t _duplex;
//     void escapedWrite(uint8_t bufferByte);
// };

/* Begin SilverSat code */
// Uncomment to enable human-readable serial0 debugging
#define DEBUG

// Debug functions:
#ifdef DEBUG

// Print a char in various representations
void debug_printchar(const char CHARACTER)
{
    Serial.println("HEX\tDEC\tBIN\tChar");
    Serial.print(CHARACTER, HEX);
    Serial.print('\t');
    Serial.print(CHARACTER, DEC);
    Serial.print('\t');
    Serial.print(CHARACTER, BIN);
    Serial.print('\t');
    Serial.println(CHARACTER);
    Serial.print('\n');
}
#endif

// Direct to Avionics command
#define CMD_AVIONICS = 0xAA

// Constants
const unsigned int BUFFERSIZE{1024}; // bytes
// const unsigned int RADIO_PACKETSIZE{256};
const unsigned int RADIO_BUFFERSIZE{BUFFERSIZE}; // Could be the AX5043 FIFO size

// Classes
// This hold the data and command byte of an unencoded KISS packet
class KISSPacket
{
private:
    // unsigned int index{0}; // Array index (shared between size() and decapsulate())
    unsigned int firstfend{0};
    unsigned int nextfend{0};

public:
    CircularBuffer<char, BUFFERSIZE> buffer; // processed incoming data
    // CircularBuffer<char, PACKETSIZE> packetbuffer; // Cut KISS packet
    bool packetfound{false}; // Whether a packet was found
    char command{CMD_DATA};  // KISS command byte

    /* Assumed cases:

        Case 1: A full and complete KISS packet exists in the buffer, possibly
                preceded by noise. This may be detected by checking for two FENDs
                seperated by at least a command byte.

            Case 1a: Multiple full and complete KISS packets are in the buffer.
                    Each packet is to be detected individually; processKISS shall
                    run in a loop until the buffer is empty.

        Case 2: Partial packets; these will be ignored.

        Case 3: Command packets; packets which solely carry a command byte. For
                every FEND detected, processKISS shall check the next byte. If the
                next byte is a FEND, processKISS shall move to it and repeat the
                check.

                Commands are defined in KISS.h; however, not all of these are
                supported. These ignored commands are those the KISS protocol lists
                as optional. In addition, the following required commands are
                ignored.

                    CMD_DATA: Any following characters, to the next FEND, are
                            assumed to be data to be sent to the serial port.
                            When this data is copied to KISSPacket, the FENDS are
                            ignored, and escape sequences are converted to their
                            normal forms.

                    CMD_TXDELAY: These are ignored because P-persistant CSMA is not
                    CMD_PERSIST: implemented. This is because the link is assumed
                    CMD_SLOTTIME: to include at most three peers on each board.

                    CMD_DUPLEX: SilverSat's current hardware does not support full
                                duplex.

                    CMD_HARDWARE: SilverSat deprecated this in favour of its own
                                custom hardware commands, to take advantage of
                                the wide range of command bytes not defined by
                                the KISS protocol.

                    */

    // Delete preceding bytes and calculate packet size
    unsigned int packetsize()
    {
        // Packet size variable
        unsigned int packetsize{1};

        while (buffer.first() != FEND)
        {
// Delete any preceding bytes
#ifdef DEBUG
            debug_printchar(buffer.shift());
#else
            buffer.shift();
#endif
        }

        if (buffer.size() > 0)
        {
            // Delete repeating FENDs
            while (buffer[1] == FEND) // ignore the next FEND
            {
#ifdef DEBUG
                debug_printchar(buffer.shift());
#else
                buffer.shift(); // Delete any preceding bytes
#endif
            }
            // firstfend = index;

            // TODO: Move to a packet processor
            // //  Get the packet command
            // packet.command = data[1];

            // In each of these cases, the next byte contains the respective data to store
            // else if (packet.command == CMD_TXDELAY)
            //     packet.txdelay = data[index + 1];
            // else if (packet.command == CMD_PERSIST)
            //     packet.P = data[index + 1];
            // else if (packet.command == CMD_SLOTTIME)
            //     packet.slottime = data[index + 1];

            // Search for the next FEND
            for (packetsize; (buffer[packetsize] != FEND); packetsize++)
            {
                #ifdef DEBUG
                debug_printchar(buffer[packetsize]);
                #endif
            }
            // nextfend = index;
        }
        return packetsize;
    }

    // Get the command
    void extractcommandbyte()
    {
        command = buffer[firstfend + 1];
    }

    // Comment: If the function did not find a KISS packet or command, packet.command will be less than 0.
    // Warning: This function only extracts one packet per run. To extract another packet, run it again.
    void decapsulate()
    {
        // Save the command byte before it is removed
        extractcommandbyte();

        // Cut the first packet out of rawdata

        // Copy the packet to the packet buffer packet only if it has two FENDS, a command byte, and at least one byte of data
        if (packetsize() >= 4)
        {
            // Copy data to packet.packet from back to front (to avoid shifting {CircularBuffer data}, for speed)
            for (unsigned int i = 0; i < nextfend; i++)
            {
                buffer.push(buffer[i + firstfend]);
            }
        }
    }
};

// Encapsulate IP data in KISS
// Input: CircularBuffer raw data, &packets
// Return: nothing
// TODO: (1) Separate to a packet size and encapsulator functions
//       (2) Change this to first convert inputdata, then pass this to kisspackets
// Note: tconrad26 keeps the KISS command between the boards
void kissencapsulate(CircularBuffer<char, RADIO_BUFFERSIZE> inputdata, CircularBuffer<char, BUFFERSIZE> &kisspackets)
{
    // Declare variables
    char databyte;

    // Start the KISS frame
    kisspackets.push(FEND);

    // Fill kisspackets with inputdata
    while ((!inputdata.isEmpty()) && (!kisspackets.isFull()))
    {
        // Shift inputdata into databyte
        databyte = inputdata.shift();

        // Check if databyte needs to be escaped
        if (databyte == FEND)
        {
            kisspackets.push(FESC);
            kisspackets.push(TFEND);
        }
        else if (databyte == FESC)
        {
            kisspackets.push(FESC);
            kisspackets.push(TFESC);
        }
        else
            kisspackets.push(databyte);
    }

    // Push a FEND to the end of kisspackets
    if (kisspackets.isFull())
    { // Replace the last byte with a FEND
        kisspackets.pop();
        kisspackets.push(FEND);
    }
    else
        kisspackets.push(FEND);
}

/* End SilverSat code*/

#endif