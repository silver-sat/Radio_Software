// Function encapsulation of Beacon 230121.int
// For now, this sends a Morse code beacon over speakerPin (defined below)
//
// Note: International Morse Code is defined here:
// https://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.1677-1-200910-I!!PDF-E.pdf
#include <string>
#include <Arduino.h> // Included for VS Code verification. It may need to be commented out before use.

class Morse
{
private:
    // define global variables
    const byte ledPin = 11;             // the pin the LED is connected to
    const byte speakerPin = 4;          // the pin the speaker is connected to
    const uint8_t BUZZERFREQUENCY{440}; // Hertz
    int duration_on{500};               // a variable for how long the tone and LED are on (in milliseconds)
    int duration_off{500};              // a variable for how long the tone and LED are off (in milliseconds)
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
        delay(duration_off);
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
    void beacon(char chartosend[])
    {
        char chartosend = tolower(*chartosend);

        Serial.begin(57600); // This will probably be done within the radio code,
                             // therefore it could be disabled. It remains for
                             // compatibility.
        // set up the LED pin as a GPIO output
        pinMode(ledPin, OUTPUT);

        // set the duration variables.
        duration_on = 500;  // milliseconds?
        duration_off = 500; // milliseconds?

        // Condition source: https://learn.microsoft.com/en-us/cpp/cpp/sizeof-operator
        for (unsigned int i = 0; i < (sizeof chartosend / sizeof chartosend[0]); i++)
        {
            Serial.println(chartosend(i));

            switch (chartosend(i))
            {
            case ' ':
                delay(duration_on * 3);
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
            case 'é':
                dit();
                dit();
                dah();
                dit();
                dit();
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
            case 'W':
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
                for (uint8_t i{0}; i < 2; i++)
                    dit();
                for (uint8_t i{0}; i < 1; i++)
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
            case '’':
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
            case '–': // en dash
                dah();
                for (uint8_t i{0}; i < 4; i++)
                    dit();
                dah();
                break;
            case '—': // em dash
                dah();
                for (uint8_t i{0}; i < 4; i++)
                    dit();
                dah();
                break;
            case '−': // minus sign
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
            case '÷':
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
            case '“': // opening double quotation marks
                dit();
                dah();
                dit();
                dit();
                dah();
                dit();
                break;
            case '”': // closing double quotation marks
                dit();
                dah();
                dit();
                dit();
                dah();
                dit();
                break;
            case '=':
                dah();
                for (uint8_t i{0}; i < 2; i++)
                    dit();
                dah();
                break;
            case 0x06: // ACK (officially "Understood")
                for (uint8_t i{0}; i < 2; i++)
                    dit();
                dah();
                dit();
                break;
            case 0x18: // Cancel (officially "Error").
                for (uint8_t i{0}; i < 8; i++)
                    dit();
                break;
            case '+':
                for (uint8_t i{0}; i < 1; i++)
                    dit();
                dah();
                dit();
                break;
            case '×':
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
            // delay(3000);
        }
    }
};