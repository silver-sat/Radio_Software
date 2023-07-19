// Test Morse_Code.h
// 2023-07-06
#include "link_to_Morse_Code.h"

void setup()
{
    // Open serial0
    Serial.begin(57600);
    while (!Serial)
        ;
    
    // Create a Morse Code instance
    Morse mymorse;
    
    // Test the Morse retriever functions
    Serial.print("ledPin == ");
    Serial.println(mymorse.getLedPin());
    Serial.print("speakerPin == ");
    Serial.println(mymorse.getSpeakerPin());
    Serial.print("buzzerFrequency == ");
    Serial.println(mymorse.speakerFrequency());
    Serial.print("Current words per minute == ");
    Serial.println(mymorse.calculateWPM());

    // Enable debug
    mymorse.debugCopyCodeToSerial(true);

    // Wait 2 seconds to begin morse code
    Serial.println("Sending test data as Morse Code in 2 seconds...\n");
    delay(2000);

    // Configure arrays
    char printablechars[79] = "The quick brown fox jumps over the lazy dog's back 123456789 times.,:?'-/\"=+@";

    // Configure non-printable characters
    const char ACK = 0x06;
    const char CANCEL = 0x18;
    char unprintablechars[2];
    unprintablechars[0] = ACK;
    unprintablechars[1] = CANCEL;

    // Send all supported characters
    mymorse.beacon(printablechars);
    mymorse.beacon(unprintablechars);
}

void loop()
{
    // Leave this blank for now.
}