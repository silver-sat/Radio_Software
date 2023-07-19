// Test Morse_Code.h
// 2023-07-06
#include "symlink_to_Morse_Code.h"

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
    Serial.println(String(mymorse.getLedPin()));
    Serial.print("speakerPin == ");
    Serial.println(String(mymorse.getSpeakerPin()));
    Serial.print("buzzerFrequency == ");
    Serial.println(String(mymorse.speakerFrequency()));
    Serial.print("Current words per minute == ");
    Serial.println(String(mymorse.calculateWPM()));

    // Enable debug
    // mymorse.debugCopyCodeToSerial(true);

    // Wait 2 seconds to begin morse code
    Serial.print("Sending test data as Morse code in 2 seconds");
    delay(500);
    Serial.print('.');
    delay(500);
    Serial.print('.');
    delay(500);
    Serial.println('.');
    delay(500);

    // Configure non-printable characters
    const char ACK = 0x06;
    const char CANCEL = 0x18;
    // Configure arrays
    char teststring[81] = "The quick brown fox jumps over the lazy dog's back 123456789 times.,:?'-/\"=+@";
    teststring[80] = ACK;
    teststring[81] = CANCEL;

    // Send all supported characters
    Serial.print("Sending '");
    Serial.print(teststring);
    Serial.println("'");
    mymorse.beacon(teststring);

    Serial.println("void setup exit");
}

void loop()
{
    // Leave this blank for now.
}