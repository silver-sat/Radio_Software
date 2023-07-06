#include <string>

// define global variables
const byte ledPin = 11;     // the pin the LED is connected to
const byte speakerPin = 4;  // the pin the speaker is connected to
int duration_on;            // a variable for how long the tone and LED are on
int duration_off;           // a variable for how long the tone and LED are off
int beeplength;
char chartosend[5] = "wila";// "NO CAPS!!" workaround implemented 2023-05-19 by isaac-silversat

void setup() {
  Serial.begin(57600);
  // set up the LED pin as a GPIO output
  pinMode(ledPin, OUTPUT);

  // set the duration variables.
  duration_on = 500;
  duration_off = 500;


}  // end of the setup function

void beep() {

  tone(speakerPin, 440);
  digitalWrite(ledPin, HIGH);
  delay(duration_on);

  // Turn off the tone and the LED, then delay for duration_off
  noTone(speakerPin);
  digitalWrite(ledPin, LOW);
  delay(duration_off);
}

void morse(int timescale = 1) {

  tone(speakerPin, 440);
  digitalWrite(ledPin, HIGH);
  delay(duration_on * timescale);

  // Turn off the tone and the LED, then delay for duration_off
  noTone(speakerPin);
  digitalWrite(ledPin, LOW);
  delay(duration_off);
}

void loop() {
  for (int i = 0; i < 4; i++) {
    Serial.println(chartosend[i]);
    delay(100);

    switch (tolower(chartosend[i])) {
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
    }

    delay(3000);
  }

}  // end of the loop function
