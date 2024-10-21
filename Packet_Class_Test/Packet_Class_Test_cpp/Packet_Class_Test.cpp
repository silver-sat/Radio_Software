#include "packet.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() 
{
  Packet testpacket;
  testpacket.packetbody = "1234 55 67";
  testpacket.packetlength = testpacket.packetbody.length() + 2; //allow for the 0xC0's and command code
  testpacket.extractParams();
  std::cout << "Number of parameters: " << testpacket.numparams << "\r\n";
  std::cout <<"Parameters : ";
  for (unsigned char i=0; i< testpacket.numparams; i++) {
    std::cout << testpacket.parameters[i] << "\r\n";
  }
  std::cout << "original packet: " << testpacket.packetbody << "\r\n";
}
