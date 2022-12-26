// Test the SAMD21G18 receive buffer size
// Written by isaac-silversat of SilverSat Ltd. on 26 December 2022

// Last modified: 2022-12-26

/* 2022-12-26 Test:

Payload of 9600 Zs sent to serial1 using an old version of serial_data_repeater
Bytes returned: 350*/

// Hardware:
const unsigned char BUTTONPIN = 13;
// Connect a push button between +5V and BUTTONPIN

// Constants
const unsigned int BAUD = 9600;

void setup()
{
    // Set BUTTONPIN mode
    pinMode(BUTTONPIN, INPUT);

    // Open the USB port
    Serial.begin(BAUD);
    Serial1.begin(BAUD);
}
void loop()
{
    // If the button is pressed, read from the serial buffer and send it back
    if (digitalRead(BUTTONPIN) == HIGH)
    {
        // Print the output of Serial.available
        Serial.print("\nSerial.available output: ");
        Serial.print(String(Serial1.available()));
        Serial.println("\n");

        // Send the receive buffer
        Serial.println("Data dump:");
        while (Serial1.available())
            Serial.write(Serial1.read());

        // Wait for a given amount of time, in milliseconds, before repeating
        delay(500);
    }
}