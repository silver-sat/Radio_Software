// Test Morse_Code.h
// 2023-07-06
#include "link_to_Morse_Code.h"

// Create a global Morse Code instance
Morse mymorse;
void setup()
{
    // Create a global Morse Code instance
    Morse mymorse;

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