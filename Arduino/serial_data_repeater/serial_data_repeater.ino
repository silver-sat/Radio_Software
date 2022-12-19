// serial_data_repeater.ino
// Resend data from one serial port to another
// Created 2021-05-05 19:00 UTC
//
// Last modified: 2022-12-19

// Libraries
#include <CircularBuffer.h>

// Constants
const int SERIAL_SPEED = 9600;
const int BUFFERSIZE = 256;

// Define variables
unsigned char serialBuffer[BUFFERSIZE];
unsigned char serial1Buffer[BUFFERSIZE];
unsigned int serial1BufPos = 0; // Array pointer for serial1Buffer
unsigned int serialBufPos = 0;  // Array pointer for serialBuffer
char inByte = '\0';

void setup()
{
    // put your setup code here, to run once:

    Serial.begin(SERIAL_SPEED);
    Serial1.begin(SERIAL_SPEED);
}

void loop()
{
    // put your main code here, to run repeatedly:

    // read from port 0 and put the incoming data into a buffer
    while (Serial.available() > 0)
    {
        inByte = Serial.read();
        serialBuffer[serialBufPos] = inByte;

        // Record the last index position of the buffer
        serialBufPos = serialBufPos + 1;
    }

    // read from port 1 and put the incoming data into a buffer
    while (Serial1.available() > 0)
    {
        inByte = Serial1.read();
        serial1Buffer[serial1BufPos] = inByte;

        // Record the last index position of the buffer
        serial1BufPos = serial1BufPos + 1;
    }

    if (serialBufPos > 0)
    {
        Serial1.write(serialBuffer, serialBufPos);
        serialBufPos = 0;
        memset(serialBuffer, 0, BUFFERSIZE);
    }

    if (serial1BufPos > 0)
    {
        Serial.write(serial1Buffer, serial1BufPos);
        serial1BufPos = 0;
        memset(serial1Buffer, 0, BUFFERSIZE);
    }
}
