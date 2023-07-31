// SilverSat Radio Board main code
// SilverSat Ltd. (https://www.silversat.org)
// 2023-02-18

// Last modified: 2023-07-11

// TODO: Consider how to process the KISS packet data.

// Tests
// GPITEST: Print all GPIO pin values to the serial port
// GPOTEST: Blink all digital GPIO pins
// #define GPITEST

// Set a loop template for GPITEST and GPOTEST
#define pinloop for (uint8_t i = 1; i < 14; i++)

#include "KISS.h"

// Include Circular Buffer library
#include <CircularBuffer.h>

// Circular Buffer
CircularBuffer<char, BUFFERSIZE> serialBuffer;// Should be a KISSPacket class

void setup()
{
#ifdef GPOTEST
    // Set all GPIO pins to output mode
    for (uint8_t i = 1; i < 14; i++)
        pinMode(i, OUTPUT);
#elif defined GPITEST
    // Set all GPIO pins to output mode
    for (uint8_t i = 1; i < 14; i++)
        pinMode(i, INPUT);
#endif

    // Open a serial port
    Serial.begin(HARDWARESERIALSPEED);
    Serial1.begin(HARDWARESERIALSPEED);

    // Print a void setup warning to detect board resets
    Serial.println("Warning: void setup() was executed.");
}

// Push data from each serial port to the other port
void loop()
{
    // put your main code here, to run repeatedly:

#ifdef GPOTEST
    // Flash all digital GPIO pins
    pinloop
        digitalWrite(i, HIGH);
    delay(1000);
    pinloop
        digitalWrite(i, LOW);

#elif defined(GPITEST)
    // Write pin outputs to serial0
    pinloop
        Serial.write(digitalRead(i));

#else

    // Read each byte from serial1 and push it to serialBuffer
    if (Serial1.available() > 0)
        serialBuffer.push(Serial1.read());

    // pass the buffer through a packet detector function here

    // (testing) Shift the buffer contents after a certain size threshold
    if (serialBuffer.size() >= 1) // Leave it 1 bytes for now
    {
        Serial.write(serialBuffer.shift());
    }

#endif
}