// serial_data_repeater.ino
// Resend data from one serial port to another
// Created 2021-05-05 19:00 UTC
//
// Last modified: 2022-12-22

// TODO: Work on CSMA and possibly move the serial transmit script in to a function
//       Last position: Line 133

#include <CircularBuffer.h> // Arduino Circular Buffer library (https://github.com/rlogiacco/CircularBuffer)
#include "KISS.h"

// Constants
const unsigned int SERIAL_SPEED = 9600;
const unsigned int BUFFERSIZE = 1024;
const unsigned int PACKETSIZE = BUFFERSIZE; // Could be the AX5043 FIFO size

// Structures
struct KISSPacket
{
    // char packet[PACKETSIZE];
    bool packetfound = false;
    char command = -1;

    // Default values are defined by the KISS standard
    unsigned char txdelay = 50;
    char P = 63;
    unsigned char slottime = 10;

    // Required by the KISS standard, but not supported by hardware.
    // Any attempt to change this will be ignored.
    // const bool fullduplex = false;
};

// Functions
// Process a KISS packet
// Input: CircularBuffer data, KISSPacket to fill
// Return: KISSPacket
// Comment: If the function did not find a KISS packet or command, packet.command will be less than 0.
// TODO: In its current state, the function gets the command of only one packet for an entire buffer.
//       Change this command to read an individual packet into KISSPacket.packet using CircularBuffer.shift()
// KISSPacket processKISS(const CircularBuffer<char, BUFFERSIZE> data, KISSPacket &packet)
// {
//     // Declare variables
//     unsigned int index = 0; // Array index

//     // Search the data for a FEND and save its index
//     do
//     {
//         if (data[index] == FEND)
//         {
//             // set the KISS command and break
//             packet.packetfound = true;
//             packet.command = data[index + 1];
//         }
//         else
//             index++;
//     } while (packet.command != FEND);

//     if (packet.packetfound)
//     {
//         // Check packet.command and take appropriate action for local commands
//         if (packet.command == FEND) // ignore the next FEND
//         {
//             index++;                      // increment the index
//             packet.command = data[index]; // get the next byte
//         }
//         // In each of these cases, the next byte contains the respective data to store
//         else if (packet.command == CMD_TXDELAY)
//             packet.txdelay = data[index + 1];
//         else if (packet.command == CMD_PERSIST)
//             packet.P = data[index + 1];
//         else if (packet.command == CMD_SLOTTIME)
//             packet.slottime = data[index + 1];
//         else
//             packet.command = -1; // Set the command to -1
//     }
// }

// Read buffers from serial ports if data is available
// Input: Reference to CircularBuffer buffer
// Return: last buffer index
unsigned int serial0readbuffer(CircularBuffer<char, BUFFERSIZE> &buffer)
{
    // Declare variables
    unsigned int index = 0;

    // Push all data from serial1 into buffer byte-by-byte
    while (Serial.available() > 0)
    {
        buffer.push(Serial.read());
        index++;
    }

    return index;
}
unsigned int serial1readbuffer(CircularBuffer<char, BUFFERSIZE> &buffer)
{
    // Declare variables
    unsigned int index = 0;

    // Push all data from serial1 into buffer byte-by-byte
    while (Serial1.available() > 0)
    {
        buffer.push(Serial1.read());
        index++;
    }

    return index;
}

// Calculate the change in time
// Input: bool reset (true to reset the counter to zero; false otherwise; default false)
// Return: long time change
long deltat(bool reset = false)
{
    // Declare variables
    static long tdif = 0,
                time = millis();

    // Set tdif
    if (reset)
        tdif = 0;
    else if (tdif > time)                  // in case of rollover
        tdif = (4294967295 - tdif) + time; // add the difference from maximim millis() to latest millis()
    else
        tdif += (time - tdif);

    return tdif;
}

void setup()
{
    // put your setup code here, to run once:

    // Open serial ports
    Serial.begin(SERIAL_SPEED);
    Serial1.begin(SERIAL_SPEED);
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

    // read from port 0 and put the incoming data into a buffer
    serialBufPos = serial0readbuffer(serialBuffer);

    // read from port 1 and put the incoming data into a buffer
    serial1BufPos = serial1readbuffer(serial1Buffer);

    if (Serial1.available() <= 0)
    { // Perform a p-persistant CSMA check
        do
        {
            // Check for data on the serial port
            if (Serial1.available() <= 0)
                rand = random(0, 255);
            else
            { // Get data for serial1 for hostPacket.slottime * 0.0001 milliseconds
                do
                {
                    serial1BufPos = serial1readbuffer(serial1Buffer);
                } while (deltat() < (static_cast<double>(hostPacket.slottime) * 0.0001));
            }
            // Reset deltat
            deltat(true);

            // If the buffer has no data, stay in the loop
            if (serial1BufPos == 0)
                rand = 0;
            else if (rand <= hostPacket.P)
            {
                // Transmit null characters for TXDELAY * 0.0001 milliseconds
                do
                    true;
                // Serial1.write("\0");
                while (deltat() < (static_cast<double>(hostPacket.txdelay) * 0.0001));

                // Send and erase serialBuffer
                for (unsigned int i = 0; i < serialBuffer.size(); i++)
                {
                    Serial1.write(serialBuffer[i]);
                }
                serialBufPos = 0; // Reset the array index
                serialBuffer.clear();
                // Reset deltat
                deltat(true);
            }
        } while (rand > hostPacket.P);
    }

    // Repeat, sending serial1Buffer to serial0
    if (Serial.available() <= 0)
    { // Perform a p-persistant CSMA check
        do
        {
            // Check for data on the serial port
            if (Serial.available() <= 0)
                rand = random(0, 255);
            else
            { // Get data for serial1 for hostPacket.slottime * 0.0001 milliseconds
                do
                {
                    serialBufPos = serial0readbuffer(serialBuffer);
                } while (deltat() < (static_cast<double>(hostPacket.slottime) * 0.0001));
            }
            // Reset deltat
            deltat(true);

            // If the buffer has no data, stay in the loop
            if (serialBufPos == 0)
                rand = 0;
            else if (rand <= hostPacket.P)
            {
                // Transmit null characters for TXDELAY * 0.0001 milliseconds
                do
                    true;
                // Serial1.write("\0");
                while (deltat() < (static_cast<double>(hostPacket.txdelay) * 0.0001));

                // Send and erase serialBuffer
                for (unsigned int i = 0; i < serialBuffer.size(); i++)
                {
                    Serial.write(serial1Buffer[i]);
                }
                serial1BufPos = 0; // Reset the array index
                serial1Buffer.clear();
                // Reset deltat
                deltat(true);
            }
        } while (rand > hostPacket.P);
    }

    // // Write serial1Buffer to serial
    // if (serial1BufPos > 0)
    // {
    //     // Send and erase serial1Buffer
    //     for (unsigned int i = 0; i < serial1Buffer.size(); i++)
    //         Serial.write(serial1Buffer[i]);
    //     serial1BufPos = 0; // Reset the array index
    //     serialBuffer.clear();
    // }
}

/* References

Chepponis, Mike, and Phil Karn "The KISS TNC: A simple Host-to-TNC
    communications protocol", January 1997 HTML version. 1987.
    https://www.ax25.net/kiss.aspx.

    Note: In the code, this is referenced by "KISS protocol" or "KISS standard".

*/