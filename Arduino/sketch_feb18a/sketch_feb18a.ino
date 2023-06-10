// SilverSat Radio Board main code
// SilverSat Ltd. (https://www.silversat.org)
// 2023-02-18

// Last modified: 2023-03-04

// TODO: Consider how to process the KISS packet data.

#include "KISS.h"

// Include Circular Buffer library
#include <CircularBuffer.h>

// Buffer Size
const unsigned int BUFFERSIZE{1024};           // bytes
const unsigned int HARDWARESERIALSPEED{57600}; // baud
const unsigned int RADIOSPEED{9600};           // baud
const unsigned int PACKETSIZE{BUFFERSIZE};     // Could be the AX5043 FIFO size

// Circular Buffer
CircularBuffer<char, BUFFERSIZE> serialBuffer;

// Classes
// This hold the data and command byte of an unencoded KISS packet
class KISSPacket
{
private:
    unsigned int index = 0; // Array index (shared between size() and decapsulate())
    unsigned int firstfend = 0;
    unsigned int nextfend = 0;
public:
    CircularBuffer<char, BUFFERSIZE> rawdata;// processed incoming data
    CircularBuffer<char, PACKETSIZE> packet; // Cut KISS packet
    unsigned int packetsize{0};              // Packet size
    bool packetfound{false};                 // Whether a packet was found
    char command{CMD_DATA};                  // KISS command byte

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

    // Get packet size
    unsigned int size()
    {
        // Search the data for a FEND and save its index
        while (packet[0] != FEND)
        {
            index++; // Increment the starting index
        }

        if (packet.size() > 0)
        {
            // Find the first FEND
            while (packet[1] == FEND) // ignore the next FEND
            {
                index++; // increment the index
            }
            firstfend = index;

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
            for (index; (packet[index] != FEND); index++)
                packetsize++;
            nextfend = index;

            return packetsize;
        }
    }

    // Get the command
    void extractcommandbyte()
    {
        command = rawdata[firstfend + 1];
    }

    // Comment: If the function did not find a KISS packet or command, packet.command will be less than 0.
    // Warning: This function only extracts one packet per run. To extract another packet, run it again.
    void decapsulate()
    {
        // Cut the first packet out of rawdata

        // Copy the packet to the packet buffer packet only if it has two FENDS, a command byte, and at least one byte of data
        if (packetsize >= 4)
        {
            // Copy data to packet.packet from back to front (to avoid shifting {CircularBuffer data}, for speed)
            for (unsigned int i = 0; i < nextfend; i++)
            {
                packet.push(rawdata[i + firstfend]);
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
void kissencapsulate(CircularBuffer<char, PACKETSIZE> inputdata, CircularBuffer<char, BUFFERSIZE> &kisspackets)
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

void setup()
{
    // put your setup code here, to run once:

    // Open a serial port
    Serial.begin(HARDWARESERIALSPEED);
    Serial1.begin(HARDWARESERIALSPEED);
}

void loop()
{
    // put your main code here, to run repeatedly:

    // Read each byte from serial1 and push it to serialBuffer
    if (Serial1.available() > 0)
    {
        serialBuffer.push(Serial1.read());
    }

    // pass the buffer through a packet detector function here

    // (testing) Shift the buffer contents after a certain size threshold
    if (serialBuffer.size() >= 1) // Leave it 1 bytes for now
    {
        Serial.write(serialBuffer.shift());
    }
}
