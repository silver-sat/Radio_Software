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
    char inByte = '\0';
    unsigned char rand = 255;

    // read from port 0 and put the incoming data into a buffer
    while (Serial.available() > 0)
    {
        inByte = Serial.read();
        // Put this first byte in a KISS packet
        serialBuffer.push(inByte);

        // Record the last index position of the buffer
        serialBufPos++;
    }

    // read from port 1 and put the incoming data into a buffer
    while (Serial1.available() > 0)
    {
        inByte = Serial1.read();
        serial1Buffer.push(inByte);

        // Record the last index position of the buffer
        serial1BufPos++;
    }

    if (Serial.available() <= 0)
    { // Perform a p-persistant CSMA check
        do
        {
            // Check for data on the serial port
            if (Serial.available() <= 0)
                rand = random(0, 255);
            else
            {
                for (unsigned long time = millis(); time < time + (hostPacket.slottime * 0.0001); time)
                {
                    // read from port 1 and put the incoming data into a buffer
                    while (Serial1.available() > 0)
                    {
                        inByte = Serial1.read();
                        serial1Buffer.push(inByte);

                        // Record the last index position of the buffer
                        serial1BufPos++;
                    }
                }
            }
            if ((rand < hostPacket.P) || (serial1BufPos == 0)) // stay in the loop
                rand = 0;
        } while (rand >= hostPacket.P);

        // Send and erase serialBuffer
        for (unsigned int i = 0; i < serialBuffer.size(); i++)
        {
            Serial1.write(serialBuffer[i]);
        }
        serialBufPos = 0; // Reset the array index
        serialBuffer.clear();
    }

    // Write serial1Buffer to serial
    if (serial1BufPos > 0)
    {
        // Send and erase serial1Buffer
        for (unsigned int i = 0; i < serial1Buffer.size(); i++)
            Serial.write(serial1Buffer[i]);
        serial1BufPos = 0; // Reset the array index
        serialBuffer.clear();
    }
}