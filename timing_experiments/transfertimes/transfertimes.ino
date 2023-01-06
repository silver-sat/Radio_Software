#include <CircularBuffer.h>

int test;
int testold;
int serialstart;
CircularBuffer<byte, 1024> buff;

void setup() {
  Serial1.begin(57600);  //start the hw serial port
  Serial.begin(57600);   //start the terminal
  Serial.print("starting serial tests");
}

void loop() {
  test = Serial1.available();  //the number of bytes in the buffer
  if (test == 0) 
  {
    serialstart = millis();
    delay(1);
  }
  /*
  //use this block to experimentally determine the size of the serial buffer  
  if (test != testold) 
  {
    testold = test;
    Serial.println(test);
  }
  */
  
  if (test == 349)  //this is the max size of the serial buffer
  {
    int serialfilled = millis() - serialstart;
    Serial.print("Serial load time: "); Serial.println(serialfilled);
    int transferstart = micros(); //ints are 4 bytes, so we're okay
    while (Serial1.available() > 0)
    {
      buff.push(Serial1.read());
    }
    int transfertime = micros() - transferstart;
    Serial.print("Transfer time: "); Serial.print(transfertime); Serial.println(" microseconds");
    while (Serial1.available() > 0)  //why is this necessary?  ans: we can empty the buffer far faster than we can fill it! so there still may be some left over
    {
      Serial1.read();  //empties the buffer
    }
  }

}
