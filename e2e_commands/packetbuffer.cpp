/**
 * @file packetbuffer.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief packet buffer library for Silversat
 * @version 1.0.1
 * @date 2022-11-08
 
 * Hierarchically, we have the following:
 * PacketBuffer: stores lots of packets, all KISS formatted
 * Packet: a KISS formatted packet, where the command is the first byte after
 *  the first delimiter
 * AX.25 Frame: the data inside a Packet (KISS removed), always an AX.25 
 *  UI packet
 * Command Frame: the data inside a Packet (KISS removed), where the destination
 *  is TBD.  

 */

#include "KISS.h"
#include <CircularBuffer.h>

class PacketBuffer
{
public:
  PacketBuffer( );
  ~PacketBuffer();
  int wrap();         //KISS format
  int unwrap();       //remove KISS
  int process();      //return length of next valid packet
  int get_command();  //returns the value of the command byte
  
protected:

private:
  CircularBuffer<unsigned char, CMDBUFFSIZE> cmdbuffer;
};