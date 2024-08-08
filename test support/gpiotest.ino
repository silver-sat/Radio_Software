/* gpiotest.ino 
 * Author: Tom Conrad
 * Silversat radio board test - checks serial ports (Serial0 and Serial1), sets up basic GPIO for radio and PA, toggles the TXLED, and scans the I2C bus
 * if TMP100 is present, it should find a device at 0x4B
 * cross connect the Serial port RXD/TXD signals (loopback).  Program writes a string and then tries to receive it.  
 * Toggles PAEnable, TxRx and RxTx lines at same time as serial port writes.

*/

#include <I2CScanner.h>
#include <Wire.h>
#include <Arduino.h>
#include <Temperature_LM75_Derived.h>

I2CScanner scanner;
  //temperature sensor
Generic_LM75_10Bit tempsense(0x4B);

void setup() {
  // put your setup code here, to run once:
  //start up the debug port
  Wire.begin();
  
  Serial.begin(57600);
  while(!Serial) {};

  //start up serial0
  Serial0.begin(57600);
  while(!Serial0) {};
  
  //start up serial1
  Serial1.begin(57600);
  while(!Serial1) {};
  
  pinMode(PIN_LED_TXL, OUTPUT); // general purpose LED
  pinMode(Release_B, OUTPUT);  //for Endurosat antenna
  pinMode(Release_A, OUTPUT);  //for Endurosat antenna
  pinMode(Current_5V, INPUT); //Analog signal that should be proportional to 5V current
  pinMode(TX_RX, OUTPUT); // TX/ RX-bar
  pinMode(RX_TX, OUTPUT); // RX/ TX-bar
  pinMode(PAENABLE, OUTPUT);  //enable the PA
  pinMode(EN0, OUTPUT);  //enable serial port differential driver
  pinMode(EN1, OUTPUT);  //enable serial port differential driver
  pinMode(AX5043_DCLK, INPUT);  //clock from the AX5043 when using wire mode
  pinMode(AX5043_DATA, OUTPUT); //data to the AX5043 when using wire mode
  pinMode(OC3V3, INPUT); //kind of a useless signal that indicates that there is an overcurrent on the 3V3 (our own supply)
  pinMode(OC5V, INPUT); //much more useful indication of an over current on the 5V supply
  pinMode(SELBAR, OUTPUT); //select for the AX5043 SPI bus
  pinMode(SYSCLK, INPUT); //AX5043 crystal oscillator clock output  

  //default RX mode state
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);
  digitalWrite(PAENABLE, LOW);

  //enable the Serial ports
  digitalWrite(EN0, HIGH);
  digitalWrite(EN1, HIGH);

  scanner.Init();

  scanner.Scan();

  digitalWrite(TX_RX, HIGH);
  digitalWrite(RX_TX, LOW);

}

void loop() {

  Serial.print("temperature of PA: "); Serial.println(tempsense.readTemperatureC());  
  delay(1000);
  // put your main code here, to run repeatedly:
/*
  digitalWrite(PIN_LED_TXL, HIGH);
  // digitalWrite(PAENABLE, HIGH);
  digitalWrite(TX_RX, LOW);
  digitalWrite(RX_TX, HIGH);
  Serial0.println("this is a test of Serial0");
  delay(1000);
  digitalWrite(PIN_LED_TXL, LOW);
  // digitalWrite(PAENABLE, LOW);
  digitalWrite(RX_TX, LOW);
  digitalWrite(TX_RX, HIGH);
  Serial1.println("this is a test of Serial1");
  delay(1000);

  Serial.println("this is a test of the serial ports");

  while (Serial0.available()) {
    Serial.println(Serial0.readString());
  }

  while(Serial1.available()) {
    Serial.println(Serial1.readString());
  }
  */
}
