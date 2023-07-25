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

    // Configure arrays
    #define LENGTH 86    // Array length
    // const unsigned char teststring[86] = "The quick brown fox jumps over the lazy dog's back 123456789 times. ", ACCENTED_E, ",:?'-/()\"=+", MULTIPLICATION_SIGN, "@";
    const char teststring[LENGTH]{'T', 'h', 'e', ' ', 'q', 'u', 'i', 'c', 'k', ' ',
                              'b', 'r', 'o', 'w', 'n', ' ', 'f', 'o', 'x', ' ',
                              'j', 'u', 'm', 'p', 's', ' ', 'o', 'v', 'e', 'r',
                              ' ', 't', 'h', 'e', 'l', 'a', 'z', 'y', ' ', 'd',
                              'o', 'g', '\'', 's', ' ', 'b', 'a', 'c', 'k', ' ',
                              '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                              ' ', 't', 'i', 'm', 'e', 's', '.', ' ', 
                              CW_ACCENTED_E, ',', ':', '?', '-', '/', '(', ')',
                              '\"', '=', ACK, CANCEL, '+',
                              CW_MULTIPLICATION_SIGN, '@'};

    // Send all supported characters
    Serial.println("Calling beacon()");
    mymorse.beacon(teststring, LENGTH);

    Serial.println("void setup exit");
}

void loop()
{
    // Leave this blank for now.
}