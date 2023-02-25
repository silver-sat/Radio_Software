/**
 * @file commands.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2022-11-08

 */

#define DEBUG
//#define MAXRESPONSE 60
//#define MAXCOMMANDSIZE 240 //this is the maximum size of a command or command response

#ifndef COMMANDS_H
#define COMMANDS_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512  //4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 //32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include <SPI.h>
#include <LibPrintf.h>
#include <CircularBuffer.h>
#include <cstdlib>  //for atoi function, may replace this with String functions, but it's working...

#include "beacon.h"
#include "KISS.h"
#include "constants.h"
#include "ax.h"

/* 

//still not convinced that a class is necessary since we're really just processing the first byte of a packet, this is a placeholder if there was something more complex like direct commands
class Commandpacket
{
  private:
    unsigned char encodedpacket[50];  
    unsigned char commandcode;
    unsigned char crcvalue[4];
    unsigned char payload[25];
  public:
    CommandPacket(CircularBuffer<unsigned char, CMDBUFFSIZE>& mybuffer, CircularBuffer<unsigned char, DATABUFFSIZE>& txbuffer, int packetlength, ax_config& config);
    ~CommandPacket();
    void processCommandBuffer();
    unsigned char getCommand();
    void getCRC();
    void getPayload();

}
*/

void sendACK(unsigned char code);
void sendNACK(unsigned char code);
void sendResponse(unsigned char code, String& response);
unsigned int deployantenna(String& response);
unsigned int reportstatus(String& response, ax_config& config);
void haltradio();
void processcmdbuff(CircularBuffer<unsigned char, CMDBUFFSIZE>& mybuffer, CircularBuffer<unsigned char, DATABUFFSIZE>& txbuffer, int packetlength, ax_config& config);

#endif

