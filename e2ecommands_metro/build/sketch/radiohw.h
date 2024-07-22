#line 1 "C:\\Users\\conra\\OneDrive\\Documents\\GitHub\\Radio_Software\\e2ecommands_metro\\radiohw.h"
/**
* @file radiohw.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing HW support for the Silversat Radio, especially the GPIO config
* @version 1.0.1
* @date 2023-7-17

radioHW.h - Library providing HW support for the Silversat Radio, especially the GPIO config
Created by Tom Conrad, July 17, 2024.
Released into the public domain.

This file defines the Radio class which provides the interface and control of the radio support components
including the TR switch and PA.  It also provides functions to switch between beacon and data modes and the underlying functions to 
send the morse characters.


*/

#ifndef RADIOHW_H
#define RADIOHW_H

//#define DEBUG

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#include "Arduino.h"

// the AX library files
#include "ax.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"

#include "constants.h"
#include "efuse.h"
#include <Temperature_LM75_Derived.h>
#include <LibPrintf.h>
#include "ExternalWatchdog.h"

class Radio
{
    public:
        Radio(int TX_RX_pin, int RX_TX_pin, int PA_enable_pin, int SYSCLK_pin, int AX5043_DCLK_pin, int AX5043_DATA_pin, int PIN_LED_TX_pin);

        void begin();
        void setTransmit(ax_config &config, ax_modulation &mod);
        void setReceive(ax_config &config, ax_modulation &mod);
        void beaconMode(ax_config &config, ax_modulation &mod);
        void dataMode(ax_config &config, ax_modulation &mod);
        void cwMode(ax_config &config, ax_modulation &mod, int duration, ExternalWatchdog &watchdog);
        size_t reportstatus(String &response, ax_config &config, ax_modulation &modulation, Efuse &efuse, bool fault);
        void dit();
        void dah();

    private: 
        int _pin_TX_RX;
        int _pin_RX_TX;
        int _pin_PAENABLE;
        int _pin_SYSCLK;
        int _pin_AX5043_DCLK;
        int _pin_AX5043_DATA;
        int _pin_TX_LED;
        pinfunc_t _func {2};  //definition of wire vs data mode
};

#endif