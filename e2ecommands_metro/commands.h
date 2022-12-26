/**
 * @file commands.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2022-11-08

 */

#define DEBUG
#define MAXRESPONSE 60
#define MAXCOMMANDSIZE 240 //this is the maximum size of a command or command response

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
#include <algorithm>
#include "beacon.h"


void sendACK(uint8_t code);
void sendNACK(uint8_t code);
void sendResponse(unsigned char& response, int responselen);
unsigned int deployantenna(unsigned char& response);
unsigned int reportstatus(unsigned char& response);
void haltradio();
void processcmdbuff(CircularBuffer<unsigned char, CMDBUFFSIZE>& mybuffer, CircularBuffer<unsigned char, DATABUFFSIZE>& txbuffer, int packetlength, ax_config& config);


#endif
