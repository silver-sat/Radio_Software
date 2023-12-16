/**
 * @file testing_support.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief testing support functions for AX5043 radio and Silversat radio board
 * @version 1.0.1
 * @date 2023-2-27

 */

#ifndef TESTING_SUPPORT_H
#define TESTING_SUPPORT_H

//#define DEBUG

#include "ax_hw.h"
#include "ax.h"
#include "ax_fifo.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"
#include "constants.h"

#include <SPI.h>
#include <LibPrintf.h>
#include <Arduino.h>

void printRegisters(ax_config& config);

#endif