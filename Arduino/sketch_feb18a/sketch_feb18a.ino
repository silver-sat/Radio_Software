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
