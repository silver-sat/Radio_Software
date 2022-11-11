/**
 * @file beacon.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief beacon generation for Silversat
 * @version 1.0.1
 * @date 2022-11-08

 */

#ifndef BEACON_H
#define BEACON_H

#define DEBUG

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#ifndef CALLSIGN
#define CALLSIGN "MYCALL" //plain reading callsign 2x3 type
#endif

#ifndef BITTIME
#define BITTIME 200
#endif

#ifndef PAdelay
#define PAdelay 1  //default to 1mSec
#endif

#include "ax.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"

//#include <stdio.h>
//#include <unistd.h>
//#include <string.h>
#include <SPI.h>
#include <LibPrintf.h>
#include <CircularBuffer.h>

void sendbeacon(unsigned char& commanddata, ax_config& config);
void dash();
void dot();



#endif /* BEACON_H */