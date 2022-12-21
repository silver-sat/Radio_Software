// serial_data_repeater.ino
// Resend data from one serial port to another
// Created 2021-05-05 19:00 UTC
//
// Last modified: 2022-12-19

// Libraries
#include <CircularBuffer.h>

// Constants
const unsigned int SERIAL_SPEED = 9600;
const unsigned int BUFFERSIZE = 256;

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
    char inByte = '\0';

    // read from port 0 and put the incoming data into a buffer
    while (Serial.available() > 0)
    {
        inByte = Serial.read();
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

    // Write serialBuffer to serial1
    if (serialBufPos > 0)
    {
        // Send and erase serialBuffer
        for (unsigned int i = 0; i < serialBufPos; i++)
            Serial1.write(serialBuffer[i]);
        serialBufPos = 0; // Reset the array index
        serialBuffer.clear();
    }

    // Write serial1Buffer to serial
    if (serial1BufPos > 0)
    {
        // Send and erase serial1Buffer
        for (unsigned int i = 0; i < serial1BufPos; i++)
            Serial.write(serial1Buffer[i]);
        serial1BufPos = 0; // Reset the array index
        serialBuffer.clear();
    }
}