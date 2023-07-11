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
    // define global variables
    byte ledPin = 13;                  // the pin the LED is connected to
    byte speakerPin = 4;               // the pin the speaker is connected to
    unsigned int buzzerFrequency{440}; // Hertz
    unsigned int duration_on{500};     // a variable for how long the tone and LED are on (in milliseconds)
    unsigned int duration_off{500};    // a variable for how long the tone and LED are off (in milliseconds)
    float dot_duty_cycle = 0.5;        // The duty cycle for a dot. This is represented as a float, not percent!
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
    // Return the dot duty cycle
    float dotDutyCycle()
    {
        return dot_duty_cycle;
    }
    // Set the dot duty cycle
    void setDotDutyCycle(float newDotDutyCycle = 0.5)
    {
        dot_duty_cycle = newDotDutyCycle;
    }

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
        return speakerPin;
    }
    // Set the buzzer's frequency
    void setSpeakerFrequency(unsigned int frequency = 440)
    {
        buzzerFrequency = frequency;
    }

    // Set words per minute
    void setWPM(unsigned int newWPM = 12)
    {
        /*
        This formula converts from words per minute (WPM) to seconds per
        letter, assuming a test word is internationally defined as PARIS
        (Bern, D.; personal communication):

        Derivation:
                                             1        1
        1 word   1 minute   word     s     ------   -----   60s
        ------ X -------- = ---- => ---- =  word  =  WPM  = ---
        minute     60 s      s      word    ----     ---    WPM
                                             s       60s

                                                  WPM
         1 s             word           second    ---    12
        ------ X -------------------- = ------ =  60s  = ---
         word    5 letters [of PARIS]   letter   -----   WPM
                                                   5
        In the next few lines of code, words per minue is calcualted and
        converted to duration_on and duration_off using percent_on. A
        second-to-millisecond conversion factor is appended.
        */
        unsigned int secondsPerLetter = 12 / newWPM;
        duration_on = static_cast<unsigned int>(float(secondsPerLetter) * dot_duty_cycle * 1000);
        duration_off = static_cast<unsigned int>(float(secondsPerLetter) * (1 - dot_duty_cycle) * 1000);
    }
    // Calculate and return current words per minute using setWPM in reverse
    unsigned int calculateWPM()
    {
        // Calculate seconds per letter
        unsigned int secondsPerLetter = duration_on / (dot_duty_cycle * 1000);

        // Calculate and return words per minute
        return 12 / secondsPerLetter;
    }

    void beacon(char chartosend[])
    {
        // Serial.begin(57600); // This will probably be done within the radio code,
        // therefore it could be disabled. It remains for
        // compatibility.
        // set up the LED pin as a GPIO output
        pinMode(ledPin, OUTPUT);

        // Condition source: https://learn.microsoft.com/en-us/cpp/cpp/sizeof-operator
        for (unsigned int i = 0; i < (sizeof chartosend / sizeof chartosend[0]); i++)
        {
            chartosend[i] = tolower(chartosend[i]); // Convert chartosend[i] to lowercase
            // Serial.println(chartosend[i]);

            switch (chartosend[i])
            {
            case ' ':
                delay(duration_on * 3);
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
            // Pad each character with a space per ITU-R M.1677-1
            delay(duration_on * 3);
        }
    }
};