# Beacon
@isaac-silversat
First version 2023-06-16

# Abstract
Satelite beacons are Morse code callsign identifications sent periodically to identify the satellite's location and frequency. For amateur radio satellites, beacons must be sent no more than ten minutes after each callsign indentification.

In Beacon230121/ , SilverSat defines a work-in-progress beacon class for use with Arduino, which is tested on an Adafruit Metro. As of this writing, it sends the string chartosend[] as Morse code over a buzzer and a LED.

# Test Setup
## Hardware
![Test schematic](Beacon230121/test_schematic/test_schematic.svg)

A buzzer and LED are connected to the pins defined in the constants buzzerPin and ledPin, respectively.

## Software
_Note: This procedure is a work in progress._

A new Arduino sketch wil be created to test Morse_Code.h . In void loop(), a pre-defined test string (which could contain all supported characters, or a shortened form) will be passed to Morse.beacon(). The Morse code output will be manually decoded. Timing will also be checked.
