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

    // Function is unused
    // void beep()
    // {

    //   tone(speakerPin, 440);
    //   digitalWrite(ledPin, HIGH);
    //   delay(duration_on);

    //   // Turn off the tone and the LED, then delay for duration_off
    //   noTone(speakerPin);
    //   digitalWrite(ledPin, LOW);
    //   delay(duration_off);
    // }

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

public:
    void beacon(char chartosend[])
    {
        char chartosend = tolower(*chartosend);

        // void setup() {
        Serial.begin(57600); // This will probably be done within the radio code,
                             // therefore it could be disabled. It remains for
                             // compatibility.
        // set up the LED pin as a GPIO output
        pinMode(ledPin, OUTPUT);

        // set the duration variables.
        duration_on = 500;  // milliseconds?
        duration_off = 500; // milliseconds?

        // }  // end of the setup function

        // void loop() {
        // for (int i = 0; i < (sizeof(chartosend)/sizeof(char)); i++) {
        //   Serial.println(chartosend[i]);
        //   delay(100);

        //   switch (chartosend[i]) {
        //     case 'a':
        //       morse();
        //       morse(3);
        //       break;
        //     case 'b':
        //       morse(3);
        //       morse();
        //       morse();
        //       morse();
        //       break;
        //     case 'c':
        //       morse(3);
        //       morse();
        //       morse(3);
        //       morse();
        //       break;
        //     case 'd':
        //       morse(3);
        //       morse();
        //       morse();
        //       break;
        //     case 'e':
        //       morse();
        //       break;
        //     case 'f':
        //       morse();
        //       morse();
        //       morse(3);
        //       morse();
        //       break;
        //     case 'g':
        //       morse(3);
        //       morse(3);
        //       morse();
        //       break;
        //     case 'h':
        //       morse();
        //       morse();
        //       morse();
        //       morse();
        //       break;
        //     case 'i':
        //       morse();
        //       morse();
        //       break;
        //     case 'j':
        //       morse();
        //       morse(3);
        //       morse(3);
        //       morse(3);
        //       break;
        //     case 'k':
        //       morse(3);
        //       morse();
        //       morse(3);
        //       break;
        //     case 'l':
        //       morse();
        //       morse(3);
        //       morse();
        //       morse();
        //       break;
        //     case 'm':
        //       morse(3);
        //       morse(3);
        //       break;
        //     case 'n':
        //       morse(3);
        //       morse();
        //       break;
        //     case 'o':
        //       morse(3);
        //       morse(3);
        //       morse(3);
        //       break;
        //     case 'p':
        //       morse();
        //       morse(3);
        //       morse(3);
        //       morse();
        //       break;
        //     case 'q':
        //       morse(3);
        //       morse(3);
        //       morse();
        //       morse(3);
        //       break;
        //     case 'r':
        //       morse();
        //       morse(3);
        //       morse();
        //       break;
        //     case 's':
        //       morse();
        //       morse();
        //       morse();
        //       break;
        //     case 't':
        //       morse(3);
        //       break;
        //     case 'u':
        //       morse();
        //       morse();
        //       morse(3);
        //       break;
        //     case 'v':
        //       morse();
        //       morse();
        //       morse();
        //       morse(3);
        //       break;
        //     case 'W':
        //     case 'w':
        //       morse();
        //       morse(3);
        //       morse(3);
        //       break;
        //     case 'x':
        //       morse(3);
        //       morse();
        //       morse();
        //       morse(3);
        //       break;
        //     case 'y':
        //       morse(3);
        //       morse();
        //       morse(3);
        //       morse(3);
        //       break;
        //     case 'z':
        //       morse(3);
        //       morse(3);
        //       morse();
        //       morse();
        //       break;
        //   }

        //   delay(3000);
        // }

        // }  // end of the loop function

        // Range-based for loop implementation
        for (auto element : chartosend)
        {
            Serial.println(element);

            switch (element)
            {
                case ' ':
                delay(duration_on * 3);
            case 'a':
                morse();
                morse(3);
                break;
            case 'b':
                morse(3);
                morse();
                morse();
                morse();
                break;
            case 'c':
                morse(3);
                morse();
                morse(3);
                morse();
                break;
            case 'd':
                morse(3);
                morse();
                morse();
                break;
            case 'e':
                morse();
                break;
            case 'é':
                morse();
                morse();
                morse(3);
                morse();
                morse();
            case 'f':
                morse();
                morse();
                morse(3);
                morse();
                break;
            case 'g':
                morse(3);
                morse(3);
                morse();
                break;
            case 'h':
                morse();
                morse();
                morse();
                morse();
                break;
            case 'i':
                morse();
                morse();
                break;
            case 'j':
                morse();
                morse(3);
                morse(3);
                morse(3);
                break;
            case 'k':
                morse(3);
                morse();
                morse(3);
                break;
            case 'l':
                morse();
                morse(3);
                morse();
                morse();
                break;
            case 'm':
                morse(3);
                morse(3);
                break;
            case 'n':
                morse(3);
                morse();
                break;
            case 'o':
                morse(3);
                morse(3);
                morse(3);
                break;
            case 'p':
                morse();
                morse(3);
                morse(3);
                morse();
                break;
            case 'q':
                morse(3);
                morse(3);
                morse();
                morse(3);
                break;
            case 'r':
                morse();
                morse(3);
                morse();
                break;
            case 's':
                morse();
                morse();
                morse();
                break;
            case 't':
                morse(3);
                break;
            case 'u':
                morse();
                morse();
                morse(3);
                break;
            case 'v':
                morse();
                morse();
                morse();
                morse(3);
                break;
            case 'W':
            case 'w':
                morse();
                morse(3);
                morse(3);
                break;
            case 'x':
                morse(3);
                morse();
                morse();
                morse(3);
                break;
            case 'y':
                morse(3);
                morse();
                morse(3);
                morse(3);
                break;
            case 'z':
                morse(3);
                morse(3);
                morse();
                morse();
                break;
            case '1':
                morse();
                for (uint8_t i{0}; i < 4; i++)
                    morse(3);
                break;
            case '2':
                morse();
                morse();
                for (uint8_t i{0}; i < 3; i++)
                    morse(3);
                break;
            case '3':
                for (uint8_t i{0}; i < 2; i++)
                    morse();
                for (uint8_t i{0}; i < 1; i++)
                    morse(3);
                break;
            case '4':
                for (uint8_t i{0}; i < 4; i++)
                    morse();
                morse(3);
                break;
            case '5':
                for (uint8_t i{0}; i < 5; i++)
                    morse();
                break;
            case '6':
                morse(3);
                for (uint8_t i{0}; i < 4; i++)
                    morse();
                break;
            case '7':
                morse(3);
                morse(3);
                for (uint8_t i{0}; i < 3; i++)
                    morse();
                break;
            case '8':
                for (uint8_t i{0}; i < 3; i++)
                    morse(3);
                morse();
                morse();
                break;
            case '9':
                for (uint8_t i{0}; i < 4; i++)
                    morse(3);
                morse();
                break;
            case '0':
                for (uint8_t i{0}; i < 5; i++)
                    morse(3);
                break;
            case '.':
                for (uint8_t i = 0; i < 3; i++)
                {
                    morse();
                    morse(3);
                }
                break;
            case ',':
                morse(3);
                morse(3);
                morse();
                morse();
                morse(3);
                morse(3);
                break;
            case ':':
                for (uint8_t i{0}; i < 3; i++)
                    morse(3);
                for (uint8_t i{0}; i < 3; i++)
                    morse();
                break;
            case '?':
                morse();
                morse();
                morse(3);
                morse(3);
                morse();
                morse();
                break;
            case '\'': // single quotation mark
                morse();
                for (uint8_t i{0}; i < 4; i++)
                    morse(3);
                morse();
                break;
            case '’':
                morse();
                for (uint8_t i{0}; i < 4; i++)
                    morse(3);
                morse();
                break;
            case '-': // hyphen
                morse(3);
                for (uint8_t i{0}; i < 4; i++)
                    morse();
                morse(3);
                break;
            case '–': // en dash
                morse(3);
                for (uint8_t i{0}; i < 4; i++)
                    morse();
                morse(3);
                break;
            case '—': // em dash
                morse(3);
                for (uint8_t i{0}; i < 4; i++)
                    morse();
                morse(3);
                break;
            case '−': // minus sign
                morse(3);
                for (uint8_t i{0}; i < 4; i++)
                    morse();
                morse(3);
                break;
            case '/':
                morse(3);
                morse();
                morse();
                morse(3);
                morse();
                break;
            case '÷':
                morse(3);
                morse();
                morse();
                morse(3);
                morse();
                break;
            case '(':
                morse(3);
                morse();
                morse(3);
                morse(3);
                morse();
                break;
            case ')':
                morse(3);
                morse();
                morse(3);
                morse(3);
                morse();
                morse(3);
                break;
            case '"':
                morse();
                morse(3);
                morse();
                morse();
                morse(3);
                morse();
                break;
            case '“': // opening double quotation marks
                morse();
                morse(3);
                morse();
                morse();
                morse(3);
                morse();
                break;
            case '”': // closing double quotation marks
                morse();
                morse(3);
                morse();
                morse();
                morse(3);
                morse();
                break;
            case '=':
                morse(3);
                for (uint8_t i{0}; i < 2; i++)
                    morse();
                morse(3);
                break;
            case 0x06: // ACK (officially "Understood")
                for (uint8_t i{0}; i < 2; i++)
                    morse();
                morse(3);
                morse();
                break;
            case 0x18: // Cancel (officially "Error").
                for (uint8_t i{0}; i < 8; i++)
                    morse();
                break;
            case '+':
                for (uint8_t i{0}; i < 1; i++)
                    morse();
                morse(3);
                morse();
                break;
            case '×':
                morse(3);
                morse();
                morse();
                morse(3);
                break;
            case '@':
                morse();
                morse(3);
                morse(3);
                morse();
                morse(3);
                morse();
                break;
            }
            // delay(3000);
        }
    }
};