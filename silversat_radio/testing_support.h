/**
 * @file testing_support.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief testing support functions for AX5043 radio and Silversat radio board
 * @version 1.0.1
 * @date 2023-2-27

 */

#ifndef TESTING_SUPPORT_H
#define TESTING_SUPPORT_H

#include "ax.h"
#include "ax_hw.h"
#include "constants.h"
#include "efuse.h"
#include "ExternalWatchdog.h"
#include "radio.h"

#include <SPI.h>
//#include <LibPrintf.h>
#include "il2p.h"
#include "il2p_rs.h"
#include "ArduinoLog.h"
#include <FastCRC.h>

//#include <Arduino.h>


void printRegisters(Radio &radio);
void efuseTesting(Efuse &efuse, ExternalWatchdog &watchdog);
void il2p_testing();

#endif