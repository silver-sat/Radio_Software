// Function encapsulation of Beacon 230121.ino
// For now, this sends a Morse code beacon over speakerPin (defined below)
//
// Note: International Morse Code is defined here:
// https://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.1677-1-200910-I!!PDF-E.pdf
#include <string>
#include <Arduino.h> // Included for VS Code verification. It may need to be commented out before use.

// Special character macros, substituting Latin-1 Extension characters with
// Unicode truncated to 8 bits
// Naming defined in ITU-R M.1677-1 § 1
const char CW_MULTIPLICATION_SIGN{0xDE}; // '×'
const char CW_ACCENTED_E{0xE7};          // 'é'
const char ACK{0x06};
const char CANCEL{0x18};

class Morse
{
private:
    // define global variables
    uint8_t ledPin = 13;               // the pin the LED is connected to
    uint8_t speakerPin = 4;            // the pin the speaker is connected to
    unsigned int buzzerFrequency{440}; // Hertz
    int duration_on{100};              // a variable for how long the tone and LED are on (in milliseconds)
    // int duration_off{100};             // a variable for how long the tone and LED are off (in milliseconds)
    // int beeplength;

    // Turn on the LED for timescale = 1 for a dot, and timescale = 3 for a dash
    void morse(int timescale = 1)
    {

        tone(speakerPin, 440);
        digitalWrite(ledPin, HIGH);
        delay(duration_on * timescale);

        // Turn off the tone and the LED, then delay for duration_off
        noTone(speakerPin);
        digitalWrite(ledPin, LOW);
        delay(duration_on);
    }

    // Morse dit and dah macros
    void dit()
    {
        morse(1);
    }
    void dah()
    {
        morse(3);
    }

public:
    // Return ledPin's value
    byte getLedPin()
    {
        return ledPin;
    }
    // Set the value of ledPin
    void setLedPin(uint8_t newPin = 13)
    {
        ledPin = newPin;
    }

    // Return speakerPin's value
    byte getSpeakerPin()
    {
        return speakerPin;
    }
    // Set the value of speakerPin
    void setSpeakerPin(uint8_t newPin = 13)
    {
        speakerPin = newPin;
    }

    // Return the buzzer's defined frequency
    unsigned int speakerFrequency()
    {
        return buzzerFrequency;
    }
    // Set the buzzer's frequency
    void setSpeakerFrequency(unsigned int frequency = 440)
    {
        buzzerFrequency = frequency;
    }

    // Set words per minute
    void setWPM(unsigned int newWPM = 12)
    {
        // Convert words per minute to milliseconds per dot and store it to dotlength
        // See wpm_to_seconds_per_dot.md for math derivation.
        duration_on = (15000 / (17 * newWPM));
    }
    // Calculate and return current words per minute using setWPM in reverse.
    // Note that, when solving for words per minute, the 50% duty cycle was
    // removed from the 15000 conversion factor.
    unsigned int calculateWPM()
    {
        // Calculate seconds per letter using setWPM in reverse, and return it
        return 30000 / (34 * duration_on);
    }

    // Send chartosend() as Morse Code
    void beacon(const char chartosend[], const uint8_t length)
    {
        Serial.begin(57600); // This will probably be done within the radio code,
                             // therefore it could be disabled. It remains for
                             // compatibility.
        // set up the LED pin as a GPIO output
        pinMode(ledPin, OUTPUT);

        // Condition source: https://learn.microsoft.com/en-us/cpp/cpp/sizeof-operator
        for (unsigned int i = 0; i < length; i++)
        {
            // Non-destructively convert chartosend to lowercase
            char letterToSend = tolower(chartosend[i]);

            // Print letterToSend to serial0, for debugging purposes
            // Format: x 0xXX
            Serial.print(letterToSend);
            Serial.print(' ');
            Serial.println(letterToSend, HEX);

            // Convert letterToSend to Morse Code using a switch statement
            switch (letterToSend)
            {
            case ' ':
                delay(duration_on * 4);
                break;
            case 'a':
                dit();
                dah();
                break;
            case 'b':
                dah();
                dit();
                dit();
                dit();
                break;
            case 'c':
                dah();
                dit();
                dah();
                dit();
                break;
            case 'd':
                dah();
                dit();
                dit();
                break;
            case 'e':
                dit();
                break;
            case CW_ACCENTED_E:
                dit();
                dit();
                dah();
                dit();
                dit();
                break;
            case 'f':
                dit();
                dit();
                dah();
                dit();
                break;
            case 'g':
                dah();
                dah();
                dit();
                break;
            case 'h':
                dit();
                dit();
                dit();
                dit();
                break;
            case 'i':
                dit();
                dit();
                break;
            case 'j':
                dit();
                dah();
                dah();
                dah();
                break;
            case 'k':
                dah();
                dit();
                dah();
                break;
            case 'l':
                dit();
                dah();
                dit();
                dit();
                break;
            case 'm':
                dah();
                dah();
                break;
            case 'n':
                dah();
                dit();
                break;
            case 'o':
                dah();
                dah();
                dah();
                break;
            case 'p':
                dit();
                dah();
                dah();
                dit();
                break;
            case 'q':
                dah();
                dah();
                dit();
                dah();
                break;
            case 'r':
                dit();
                dah();
                dit();
                break;
            case 's':
                dit();
                dit();
                dit();
                break;
            case 't':
                dah();
                break;
            case 'u':
                dit();
                dit();
                dah();
                break;
            case 'v':
                dit();
                dit();
                dit();
                dah();
                break;
            case 'w':
                dit();
                dah();
                dah();
                break;
            case 'x':
                dah();
                dit();
                dit();
                dah();
                break;
            case 'y':
                dah();
                dit();
                dah();
                dah();
                break;
            case 'z':
                dah();
                dah();
                dit();
                dit();
                break;
            case '1':
                dit();
                for (uint8_t i{0}; i < 4; i++)
                    dah();
                break;
            case '2':
                dit();
                dit();
                for (uint8_t i{0}; i < 3; i++)
                    dah();
                break;
            case '3':
                for (uint8_t i{0}; i < 3; i++)
                    dit();
                for (uint8_t i{0}; i < 2; i++)
                    dah();
                break;
            case '4':
                for (uint8_t i{0}; i < 4; i++)
                    dit();
                dah();
                break;
            case '5':
                for (uint8_t i{0}; i < 5; i++)
                    dit();
                break;
            case '6':
                dah();
                for (uint8_t i{0}; i < 4; i++)
                    dit();
                break;
            case '7':
                dah();
                dah();
                for (uint8_t i{0}; i < 3; i++)
                    dit();
                break;
            case '8':
                for (uint8_t i{0}; i < 3; i++)
                    dah();
                dit();
                dit();
                break;
            case '9':
                for (uint8_t i{0}; i < 4; i++)
                    dah();
                dit();
                break;
            case '0':
                for (uint8_t i{0}; i < 5; i++)
                    dah();
                break;
            case '.':
                for (uint8_t i = 0; i < 3; i++)
                {
                    dit();
                    dah();
                }
                break;
            case ',':
                dah();
                dah();
                dit();
                dit();
                dah();
                dah();
                break;
            case ':':
                for (uint8_t i{0}; i < 3; i++)
                    dah();
                for (uint8_t i{0}; i < 3; i++)
                    dit();
                break;
            case '?':
                dit();
                dit();
                dah();
                dah();
                dit();
                dit();
                break;
            case '\'': // single quotation mark
                dit();
                for (uint8_t i{0}; i < 4; i++)
                    dah();
                dit();
                break;
            case '-': // hyphen
                dah();
                for (uint8_t i{0}; i < 4; i++)
                    dit();
                dah();
                break;
            case '/':
                dah();
                dit();
                dit();
                dah();
                dit();
                break;
            case '(':
                dah();
                dit();
                dah();
                dah();
                dit();
                break;
            case ')':
                dah();
                dit();
                dah();
                dah();
                dit();
                dah();
                break;
            case '"':
                dit();
                dah();
                dit();
                dit();
                dah();
                dit();
                break;
            case '=':
                dah();
                for (uint8_t i{0}; i < 3; i++)
                    dit();
                dah();
                break;
            case 0x06: // ACK (officially "Understood")
                for (uint8_t i{0}; i < 3; i++)
                    dit();
                dah();
                dit();
                break;
            case 0x18: // Cancel (officially "Error").
                for (uint8_t i{0}; i < 8; i++)
                    dit();
                break;
            case '+':
                for (uint8_t i{0}; i < 2; i++)
                {
                    dit();
                    dah();
                }
                dit();
                break;
            case CW_MULTIPLICATION_SIGN:
                dah();
                dit();
                dit();
                dah();
                break;
            case '@':
                dit();
                dah();
                dah();
                dit();
                dah();
                dit();
                break;
            }
            delay(duration_on * 3); // ITU-R M.1677-1 §2.3 letter spacing
        }
    }
};
