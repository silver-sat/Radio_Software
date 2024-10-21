// Test Morse_Code.h
// 2023-07-06
#include "Morse_Code.h"

char background_S_level()
{
    // Get the background RSSI
    // int RSSI{background_rssi(commandpacket, config, modulation, radio, watchdog)};

    // Suppose the background RSSI is -100 dBm for now
    int RSSI{-100};
    char S_level;

    // For the purposes of the beacon, ensure the S-meter level is between 0 and 9 (including endpoints)
    if (RSSI < -121)
        char S_level{0};
    else if (RSSI > -73)
        char S_level{9};
    else
    {
        // The relationship between dBm and RSSI is linear from S1 to S9, so use linear scaling.
        // Model: y = mx + b
        // m = (S9-S1)/(-73-121 dBm)=8/48 = 1/6
        // Using S9, 9 = -73m + b => b = 9 + 73m = 127/6
        const double B{127/6};
        char S_level{static_cast<char>(RSSI/6 + B)};
    }

    return S_level;
}

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
// #define LENGTH 86 // Array length
    // const unsigned char teststring[86] = "The quick brown fox jumps over the lazy dog's back 123456789 times. ", ACCENTED_E, ",:?'-/()\"=+", MULTIPLICATION_SIGN, "@";
    // const char teststring[LENGTH]{'T', 'h', 'e', ' ', 'q', 'u', 'i', 'c', 'k', ' ',
    //                               'b', 'r', 'o', 'w', 'n', ' ', 'f', 'o', 'x', ' ',
    //                               'j', 'u', 'm', 'p', 's', ' ', 'o', 'v', 'e', 'r',
    //                               ' ', 't', 'h', 'e', ' ', 'l', 'a', 'z', 'y', ' ',
    //                               'd', 'o', 'g', '\'', 's', ' ', 'b', 'a', 'c',
    //                               'k', ' ', '1', '2', '3', '4', '5', '6', '7', '8',
    //                               '9', '0', ' ', 't', 'i', 'm', 'e', 's', '.', ' ',
    //                               CW_ACCENTED_E, ',', ':', '?', '-', '/', '(', ')',
    //                               '\"', '=', ACK, CANCEL, '+','@'};
    const unsigned char LENGTH{2};
    char beacondata[LENGTH];
    
    // Test the beacon character from commands.cpp
    // Generate radio beacon character
    // For now, only consider the S-meter level. Other error conditions will be added later
    // Written by isaac-silversat, 2024-07-30

    beacondata[0] = background_S_level() + 0x30; // placeholder for radio status byte

    beacondata[1] = 0;    // add null terminator
    // int beaconstringlength = sizeof(beacondata);
    // debug_printf("beacondata = %12c \r\n", beacondata);

    // sendbeacon(beacondata, beaconstringlength, config, modulation, watchdog, efuse, radio);

    // Send all supported characters
    Serial.println("Calling beacon()");
    mymorse.beacon(beacondata, LENGTH);

    Serial.println("void setup exit");
}

void loop()
{
    // Leave this blank for now.
}