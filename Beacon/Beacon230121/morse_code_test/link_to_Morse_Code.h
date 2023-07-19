// Function encapsulation of Beacon 230121.int
// For now, this sends a Morse code beacon over speakerPin (defined below)
//
// Note: International Morse Code is defined in ITU-R M.1677-1
// (https://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.1677-1-200910-I!!PDF-E.pdf)
#include <string>
#include <Arduino.h> // Included for VS Code verification. It may need to be commented out before use.

class Morse
{
private:
    // Configuration variables
    bool debugSerialCodeCopy{false};   // Whether to copy the Morse code output to the serial port
    byte ledPin{13};                   // the pin the LED is connected to
    byte speakerPin{4};                // the pin the speaker is connected to
    unsigned int buzzerFrequency{440}; // Hertz
    unsigned int dotlength{500};       // The length of a dot, in milliseconds.
    unsigned int serialSpeed{57600};   // Debug serial speed

    // Turn on the LED for timescale = 1 for a dot, and timescale = 3 for a dash
    void morse(int timescale = 1)
    {

        tone(speakerPin, 440);
        digitalWrite(ledPin, HIGH);
        delay(dotlength * timescale);

        // Turn off the tone and the LED, then delay for duration_off
        noTone(speakerPin);
        digitalWrite(ledPin, LOW);
        delay(dotlength);
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
    void space()
    {
        for (byte i = 0; i < 3; i++)
        {
            delay(dotlength);
        }
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
        dotlength = (15000 / (17 * newWPM));
    }
    // Calculate and return current words per minute using setWPM in reverse.
    // Note that, when solving for words per minute, the 50% duty cycle was
    // removed from the 15000 conversion factor.
    unsigned int calculateWPM()
    {
        // Calculate seconds per letter using setWPM in reverse, and return it
        return 30000 / (34 * dotlength);
    }

    // Set whether to copy the Morse output to the serial port (Default off)
    void debugCopyCodeToSerial(bool newStatus = false, unsigned int newSerialSpeed = 57600)
    {
        debugSerialCodeCopy = newStatus;
        serialSpeed = newSerialSpeed;
    }

    void beacon(char chartosend[])
    {
        // set up the LED pin as a GPIO output
        pinMode(ledPin, OUTPUT);

        // Indicate setup by quickly flashing the LED
        tone(ledPin, 10);

        // // For the serial debug
        // if (debugSerialCodeCopy)
        //     // If the serial0 is not initialized, initialize it
        //     if (!Serial)
        //         Serial.begin(serialSpeed);
        
        // Stop the setup indicator
        noTone(ledPin);

        // Condition source: https://learn.microsoft.com/en-us/cpp/cpp/sizeof-operator
        // Its associated compiler warning can be safely ignored because it is
        // compensated for by dividing the size of the first element.
        for (unsigned int i = 0; i < (sizeof chartosend / sizeof chartosend[0]); i++)
        {
            chartosend[i] = tolower(chartosend[i]); // Convert chartosend[i] to lowercase

            // Special spacing between words per ITU-R M.1677-1 § 2.4, to avoid interfering with § 2.3. Thus, a word space will not use a letter space.
            if (chartosend[i] = ' ')
                delay(dotlength * 7);
            else
            {
                switch (chartosend[i])
                {
                // case ' ': // Moved out of switch statement
                //     delay(duration_on * 7); // See ITU-R M.1677-1 § 2.4
                //     break;
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
                // case 'é':    // Unsupported by ASCII
                //     dit();
                //     dit();
                //     dah();
                //     dit();
                //     dit();
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
                // case '’':    // Unsupported by ASCII
                //     dit();
                //     for (uint8_t i{0}; i < 4; i++)
                //         dah();
                //     dit();
                //     break;
                case '-': // hyphen
                    dah();
                    for (uint8_t i{0}; i < 4; i++)
                        dit();
                    dah();
                    break;
                // case '–': // en dash. Unsupported by ASCII
                //     dah();
                //     for (uint8_t i{0}; i < 4; i++)
                //         dit();
                //     dah();
                //     break;
                // case '—': // em dash. Unsupported by ASCII
                //     dah();
                //     for (uint8_t i{0}; i < 4; i++)
                //         dit();
                //     dah();
                //     break;
                // case '−': // minus sign. Unsupported by ASCII
                //     dah();
                //     for (uint8_t i{0}; i < 4; i++)
                //         dit();
                //     dah();
                //     break;
                case '/':
                    dah();
                    dit();
                    dit();
                    dah();
                    dit();
                    break;
                // case '÷':    // Unsupported by ASCII
                //     dah();
                //     dit();
                //     dit();
                //     dah();
                //     dit();
                //     break;
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
                // case '“': // opening double quotation marks. Unsupported by ASCII
                //     dit();
                //     dah();
                //     dit();
                //     dit();
                //     dah();
                //     dit();
                //     break;
                // case '”': // closing double quotation marks. Unsupported by ASCII
                //     dit();
                //     dah();
                //     dit();
                //     dit();
                //     dah();
                //     dit();
                //     break;
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
                // case '×':    // Multiplication sign. Unsupported by ASCII
                //     dah();
                //     dit();
                //     dit();
                //     dah();
                //     break;
                case '@':
                    dit();
                    dah();
                    dah();
                    dit();
                    dah();
                    dit();
                    break;
                }
                // Pad each character with a space per ITU-R M.1677-1 § 2.3
                delay(dotlength * 3);

                if (debugSerialCodeCopy)
                    // Print the last sent character to the serial port
                    Serial.println(chartosend[i]);
            }
        }
    }
};

/* References
Adafruit (2021). strandtest. GitHub
    https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
    Accessed 19 July 2023'
*/