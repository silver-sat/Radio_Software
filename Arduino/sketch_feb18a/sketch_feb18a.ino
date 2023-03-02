// SilverSat Radio Board main code
// SilverSat Ltd. (https://www.silversat.org)
// 2023-02-18

// Last modified: 2023-02-25

// Include Circular Buffer library
#include <CircularBuffer.h>

// Buffer Size
const unsigned int BUFFERSIZE = 1024; // bytes
const unsigned int HARDWARESERIALSPEED = 576000; // baud

// Circular Buffer
CircularBuffer<char, BUFFERSIZE> serialBuffer;


void setup() {
  // put your setup code here, to run once:

  // Open a serial port
  Serial.begin(HARDWARESERIALSPEED);
  Serial1.begin(HARDWARESERIALSPEED);

}

void loop() {
  // put your main code here, to run repeatedly:

  // Read each byte from serial1 and push it to serialBuffer
  if (Serial1.avalailble() > 0)
  serialBuffer.push(Serial1.read());

  // pass the buffer through a packet detector function here

  // (testing) Shift the buffer contents after a certain size threshold
  if (serialBuffer.size() >= 5 && serialBuffer.size() != 0) // Leave it 5 bytes for now
  {
    Serial.write(serialBuffer.shift());
  }
}
