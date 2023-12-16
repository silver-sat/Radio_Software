/**
 * @file beacon.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief beacon generation for Silversat
 * @version 1.0.1
 * @date 2022-11-08

 */

#ifndef BEACON_H
#define BEACON_H

// #define DEBUG

#include "ax.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"
#include "constants.h"

#include <SPI.h>
#include <LibPrintf.h>
#include <CircularBuffer.h>
#include <Arduino.h>

void sendbeacon(byte beacondata[], int beaconstringlength, ax_config& config, ax_modulation& modulation);
void dah();
void dit();


#endif /* BEACON_H */