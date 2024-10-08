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

#include "radio.h"

//#include <LibPrintf.h>
#include <ArduinoLog.h>
#include "Arduino.h"

//packet class for silversat radio packets
class Packet
{
public:
  Packet();
  unsigned char commandcode;
  int packetlength {0};

  char packetbody[30];
  String parameters[4];  //an array of command parameters
  int numparams {0};
  
  int extractParams(); 

private:
  
};

#endif