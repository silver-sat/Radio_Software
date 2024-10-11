/**
 * @file packet_processor.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief packet processor for Silversat
 * @version 1.0.1
 * @date 2024-10-07
 *
 * This code doesn't actually do anything yet.  It's more of a concept toward storing unencoded packets in the 
 * buffers as packet objects rather than kiss.  
 *
 */


#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512 // 4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 // 32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include "beacon.h"
#include "KISS.h"
#include "constants.h"
#include "ExternalWatchdog.h"
#include "radio.h"
#include "packet.h"

#include <SPI.h>
#include <Wire.h>
#include <CircularBuffer.hpp>
#include <ArduinoLog.h>

#include "Arduino.h"


// determines if packet is destined for other end of link, and if not, extracts the body
bool processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, CommandPacket &commandpacket);

//extracts the header and subheader fields from a packet
bool processdatabuff(CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, DataPacket &datapacket);


#endif