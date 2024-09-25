/**
* @file packet.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library for using the packet handling
* @version 1.0.1
* @date 2024-9-17

packet.h - Library for using packets
Created by Tom Conrad, September 17, 2024.
Released into the public domain.


*/

#ifndef PACKET_H
#define PACKET_H

#define DEBUG

#include <LibPrintf.h>

#include <vector>
#include <string>

//#include "Arduino.h"

//packet class for silversat radio packets
class Packet
{
public:
  Packet();
  unsigned char commandcode;
  int packetlength {0};
  std::string packetbody{};
  std::vector<std::string> parameters;  //an array of command parameters
  int numparams {0};
  
  int extractParams();  //pop

private:
  
};

#endif