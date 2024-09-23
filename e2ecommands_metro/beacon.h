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
#include "ax_modes.h"

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

void sendbeacon(byte beacondata[], int beaconstringlength, ExternalWatchdog &watchdog, Efuse &efuse, Radio &radio);
void dah(Radio &radio, Efuse &efuse);
void dit(Radio &radio, Efuse &efuse);

#endif // BEACON_H