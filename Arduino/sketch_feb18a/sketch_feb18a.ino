// SilverSat Radio Board main code
// SilverSat Ltd. (https://www.silversat.org)
// 2023-02-18

// TODO: Consider how to process the KISS packet data.

// Tests
// GPITEST: Print all GPIO pin values to the serial port
// GPOTEST: Blink all digital GPIO pins
// #define GPITEST

// Set a loop template for GPITEST and GPOTEST
#define pinloop for (uint8_t i = 1; i < 14; i++)

// Include Circular Buffer library
#include <CircularBuffer.h>

#include "KISS.h"

// Serial port speeds
const unsigned int HARDWARESERIALSPEED{57600}; // baud
const unsigned int RADIOSPEED{9600};           // baud

// Circular Buffer
KISSPacket serial0Buffer;
KISSPacket serial1Buffer;

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
    /* Serial1 to Serial0 transfer */
    // Read each byte from serial1 and push it to serialBuffer
    if (Serial1.available() > 0)
        serial0Buffer.buffer.push(Serial1.read());

    // pass the buffer through a packet detector function here

    // (testing) Shift the buffer contents after a certain size threshold
    // First, check if the buffer'ss side is greater than 3 (two FENDs, a command byte, and at least one data byte). This declutters the serial monitor when debug is used.
    // Next, check if the packet itself is greater than 3 bytes
    if ((serial0Buffer.buffer.size() > 3) && (serial0Buffer.packetsize() > 3))
        Serial.write(serial0Buffer.buffer.shift());

    /* Serial0 to Serial1 transfer */
    // Read each byte from serial0 and push it to serialBuffer
    if (Serial.available() > 0)
        serial1Buffer.buffer.push(Serial.read());

    // pass the buffer through a packet detector function here

    // (testing) Shift the buffer contents after a certain size threshold
    if ((serial1Buffer.buffer.size() > 3) && (serial1Buffer.packetsize() > 0)) // Leave it 1 bytes for now
        Serial1.write(serial1Buffer.buffer.shift());

#endif
}