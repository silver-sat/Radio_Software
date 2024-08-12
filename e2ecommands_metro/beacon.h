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

#include "constants.h"
#include "ExternalWatchdog.h"
#include "efuse.h"
#include "radio.h"

#include <SPI.h>
#include <LibPrintf.h>
#include <Arduino.h>

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

void sendbeacon(byte beacondata[], int beaconstringlength, ax_config &config, ax_modulation &modulation, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio);
void dah(Radio &radio);
void dit(Radio &radio);

#endif /* BEACON_H */