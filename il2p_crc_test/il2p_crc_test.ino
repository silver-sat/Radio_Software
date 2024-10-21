

#include "il2p_crc.h"


uint8_t buf[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

FastCRC16 CRC16;
IL2P_CRC il2p_crc;


void setup() {
  Serial.begin(115200);
  while(!Serial);
  
  uint32_t il2p_word = il2p_crc.calculate(buf, 9);
  
  Serial.print("the CRC is: "); Serial.println(il2p_word, BIN);

  if (il2p_crc.verify(buf, 9,il2p_word)) Serial.println("Verified!");
  else Serial.println("No Joy!");
  
}


void loop() {
  // put your main code here, to run repeatedly:

}
