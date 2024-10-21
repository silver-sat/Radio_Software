#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\packet.h"
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
#include <ArduinoLog.h>
#include "Arduino.h"
#include "ax.h"
#include "CircularBuffer.h"

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512 // 4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 // 32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

//this is the basic packet class.  I want to add derived classes for commands, data and il2p packets.
//probably want to add the kiss functions as well as the basic packet finding classes
//ultimately I'd prefer to store packet class objects rather than kiss formatted bytes

class Packet
{
  public:
    Packet();
    unsigned char commandcode;
    int packetlength {0};

    unsigned char packetbody[255];  //the entire kiss unwrapped body, with the command code
    String parameters[4];  //an array of command parameters
    int numparams {0};

    int extractParams();
    bool processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer);

  private:
    int m_sent; //have I been sent?  this is for a future connected mode layer

};

#endif