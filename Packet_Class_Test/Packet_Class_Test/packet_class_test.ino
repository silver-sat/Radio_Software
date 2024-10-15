  #include "packet.h"

void setup() {
  Serial.begin(57600);
  while(!Serial);
  
  Packet testpacket;
  testpacket.packetbody = "1234 55 67";
  testpacket.packetlength = testpacket.packetbody.length() + 2; //allow for the 0xC0's and command code
  testpacket.extractParams();
  Serial.print("Number of parameters: "); Serial.println(testpacket.numparams);
  Serial.println("Parameters : ");
  for (int i=0; i< testpacket.numparams; i++) {
    Serial.println(testpacket.parameters[i]);
  }
  Serial.print("original packet: "); Serial.println(testpacket.packetbody);
}

void loop() {
  // put your main code here, to run repeatedly:

}
