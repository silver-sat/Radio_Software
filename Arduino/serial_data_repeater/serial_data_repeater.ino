// serial_data_repeater.ino
// Resend data from one serial port to another
// Created 2021-05-05 19:00 UTC
//
// Last modified: 2022-12-30

#include <CircularBuffer.h> // Arduino Circular Buffer library (https://github.com/rlogiacco/CircularBuffer)
#include <climits>          // STDC++ data type extremes library
#include "KISS.h"

// Constants
const unsigned int SERIAL_SPEED = 9600;
const unsigned int BUFFERSIZE = 1024;
const unsigned int PACKETSIZE = BUFFERSIZE; // Could be the AX5043 FIFO size
const unsigned char LEDPIN = 13,
                    LEDPIN_SERIAL1 = 12;

// Global variables (copied verbatim from Mellis et al.)

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0; // will store last time LED was updated

// Structures
struct KISSPacket
{
    char packet[PACKETSIZE];
    bool packetfound = false;
    char command = -1;

    // Default values are defined by the KISS standard
    // unsigned char txdelay = 50;  // default 50
    // char P = 63;                 // default 63
    // unsigned char slottime = 10; // default 10

    // Required by the KISS standard, but not supported by hardware.
    // Any attempt to change this will be ignored.
    // const bool fullduplex = false;
};

// Functions
// Process a KISS packet
// Input: CircularBuffer data, KISSPacket to fill
// Return: nothing
// Comment: If the function did not find a KISS packet or command, packet.command will be less than 0.
// Warning: This function only extracts one packet per run. To extract another packet, run it again.
void processKISS(CircularBuffer<char, BUFFERSIZE> data, KISSPacket &packet)
{
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

                              SilverSat has not yet published these commands to
                              prevent unauthorized administrative access to the
                              satellite.*/

    // Declare variables
    unsigned int index = 0; // Array index

    // Search the data for a FEND and save its index
    do
    {
        if (data[index] == FEND)
        {
            // set and check the KISS command
            packet.packetfound = true;
            packet.command = data[index + 1];
        }
        else
            index++;
    } while (data[index - 1] != FEND);

    if (packet.packetfound)
    {
        // Check packet.command and take appropriate action for local commands
        if (packet.command == FEND) // ignore the next FEND
        {
            index++;                      // increment the index
            packet.command = data[index]; // get the next byte
        }
        // In each of these cases, the next byte contains the respective data to store
        // else if (packet.command == CMD_TXDELAY)
        //     packet.txdelay = data[index + 1];
        // else if (packet.command == CMD_PERSIST)
        //     packet.P = data[index + 1];
        // else if (packet.command == CMD_SLOTTIME)
        //     packet.slottime = data[index + 1];
        if (packet.command == CMD_DATA)
        {
            // Search for the next FEND
            unsigned int nextFEND = index + 1; // Temporary storage variable
            for (nextFEND; (data[nextFEND] != FEND) && (nextFEND < PACKETSIZE); nextFEND++)
                ; // The full evaluation should be performed in the above line

            // Copy this packet to KISSPacket packet only if another FEND was found
            if (nextFEND < (PACKETSIZE - 1))
            {
                for (int i = index, j = 0; i < nextFEND; i++)
                { // Ignore FENDS
                    if (data[i] == FEND)
                        index++;
                    else
                    {
                        // Check for transposed bytes
                        if (data[i] == FESC)
                        {
                            if (data[i + 1] == TFEND)
                                packet.packet[j] = FEND;
                            else if (data[i + 1] == TFESC)
                                packet.packet[j] = FESC;
                        }
                        else // copy the byte to packet.packet
                            packet.packet[j] = data[i];

                        // Increment j
                        j++;
                    }
                }

                // Clear the data up to nextFEND
                for (int i = 0; (i < nextFEND + 1) && (nextFEND + 1 < PACKETSIZE); i++)
                {
                    data.shift();
                }
                
            }
            else // in case 2, clear the buffer and set data to indicate such
            {
                data.clear();
                packet.packetfound = false;
                packet.command = -1;
            }
        }
        else
            packet.command = -1; // Set the command to -1
    }
}

// Read buffers from serial ports if data is available
// Input: Reference to CircularBuffer buffer
// Return: last buffer index
unsigned int serial0readbuffer(CircularBuffer<char, BUFFERSIZE> &buffer)
{
    // Declare variables
    unsigned int index = 0;

    // Turn on an LED indicator
    if (Serial.available() > 0)
        digitalWrite(LEDPIN, HIGH);

    // Push all data from serial1 into buffer byte-by-byte
    while (Serial.available() > 0)
    {
        buffer.push(Serial.read());
        index++;
    }

    digitalWrite(LEDPIN, LOW); // turn the indicator off

    return index;
}
unsigned int serial1readbuffer(CircularBuffer<char, BUFFERSIZE> &buffer)
{
    // Declare variables
    unsigned int index = 0;

    // Turn on an LED indicator
    if (Serial.available() > 0)
        digitalWrite(LEDPIN, HIGH);

    // Push all data from serial1 into buffer byte-by-byte
    while (Serial1.available() > 0)
    {
        buffer.push(Serial1.read());
        index++;
    }

    digitalWrite(LEDPIN_SERIAL1, LOW); // turn the indicator off

    return index;
}

// Time-delay (modified) from Mellins et al.
// Input: long last time difference
// Return: bool repeat loop
// Comments: Use as a loop condition; the loop contains the code to execute within this time.
bool repeat(unsigned long interval)
{
    // Declare variables
    unsigned long currentMillis = millis();

    // else if (previousMillis > currentMillis) // in case of rollover
    //     interval = (LONG_MAX - tdif) + time;     // add the difference from maximim millis() to latest millis()
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        return true; // stop the function here
    }

    return false; // this should only be executed if the above condition is not satisfied
}

void setup()
{
    // put your setup code here, to run once:

    // Open serial ports
    Serial.begin(SERIAL_SPEED);
    Serial1.begin(SERIAL_SPEED);

    // Set the LED pin mode
    pinMode(LEDPIN, OUTPUT);
    pinMode(LEDPIN_SERIAL1, OUTPUT);
}

void loop()
{
    // put your main code here, to run repeatedly:

    // Declare variables
    CircularBuffer<char, BUFFERSIZE> serialBuffer;
    CircularBuffer<char, BUFFERSIZE> serial1Buffer;
    unsigned int serial1BufPos = 0; // Array pointer for seria1lArray
    unsigned int serialBufPos = 0;  // Array pointer for serialArray
    KISSPacket hostPacket;
    KISSPacket radioPacket;
    unsigned char rand = 255;

    digitalWrite(LEDPIN, LOW); // turn the LED off after executing the while loop

    // read from port 0 and put the incoming data into a buffer
    serialBufPos = serial0readbuffer(serialBuffer);

    // read from port 1 and put the incoming data into a buffer
    serial1BufPos = serial1readbuffer(serial1Buffer);

    // Write serial1Buffer to serial1, only if no data is coming in to serial0
    if ((serial1BufPos > 0) && (Serial.available() != -1))
    {
        // Send and erase serial1Buffer
        for (unsigned int i = 0; i < serial1Buffer.size(); i++)
            Serial.write(serial1Buffer[i]);
        serial1BufPos = 0; // Reset the array index
        serial1Buffer.clear();
    }

    // Write serialBuffer to serial1, only if no data is coming in to serial1
    if ((serialBufPos > 0) && (Serial1.available() != -1))
    {
        // Send and erase serialBuffer
        for (unsigned int i = 0; i < serialBuffer.size(); i++)
            Serial1.write(serialBuffer[i]);
        serialBufPos = 0; // Reset the array index
        serialBuffer.clear();
    }
}

/* References

Chepponis, Mike, and Phil Karn "The KISS TNC: A simple Host-to-TNC
    communications protocol", January 1997 HTML version. 1987.
    https://www.ax25.net/kiss.aspx.

    Note: In the code, this is referenced by "KISS protocol" or "KISS standard".

*/