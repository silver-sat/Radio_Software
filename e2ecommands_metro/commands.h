/**
 * @file commands.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief command processor for Silversat
 * @version 1.0.1
 * @date 2022-11-08

 */

//#define DEBUG

#ifndef COMMANDS_H
#define COMMANDS_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512  //4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 //32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include <SPI.h>
#include <Wire.h>
#include <LibPrintf.h>
#include <CircularBuffer.h>
#include <cstdlib>  //for atoi function, may replace this with String functions, but it's working...
#include <Arduino.h>

#include "beacon.h"
#include "KISS.h"
#include "constants.h"
#include "ax.h"
#include <Temperature_LM75_Derived.h>
#include "ExternalWatchdog.h"
#include "efuse.h"

void sendACK(byte code);
void sendNACK(byte code);
void sendResponse(byte code, String& response);
size_t reportstatus(String& response, ax_config& config, ax_modulation& modulation);
void processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, ax_config &config, ax_modulation &modulation, bool &transmit, int &offset, ExternalWatchdog &watchdog);
#endif

