/**
* @file radio.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing HW support for the Silversat Radio, especially the GPIO config
* @version 1.0.1
* @date 2024-7-24

radio.h - Library providing HW support for the Silversat Radio, especially the GPIO config
Created by Tom Conrad, July 17, 2024.
Released into the public domain.

This file defines the Radio class which provides the interface and control of the radio support components
including the TR switch and PA.  It also provides functions to switch between beacon and data modes and the underlying functions to 
send the morse characters.


*/

#ifndef RADIO_H
#define RADIO_H

#define DEBUG

#include "Arduino.h"
#include "ax.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "constants.h"
#include "efuse.h"
#include <Temperature_LM75_Derived.h>
#include <LibPrintf.h>
#include "ExternalWatchdog.h"
#include <FlashStorage.h>

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

class Radio
{
    public:
        Radio(int TX_RX_pin, int RX_TX_pin, int PA_enable_pin, int SYSCLK_pin, int AX5043_DCLK_pin, int AX5043_DATA_pin, int PIN_LED_TX_pin);

        void begin(ax_config &config, ax_modulation &mod, void (*spi_transfer)(unsigned char *, uint8_t), FlashStorageClass<int> &operating_frequency);
        void setTransmit(ax_config &config, ax_modulation &mod);
        void setReceive(ax_config &config, ax_modulation &mod);
        void beaconMode(ax_config &config, ax_modulation &mod);
        void dataMode(ax_config &config, ax_modulation &mod);
        void cwMode(ax_config &config, ax_modulation &mod, int duration, ExternalWatchdog &watchdog);
        size_t reportstatus(String &response, ax_config &config, ax_modulation &modulation, Efuse &efuse, bool fault);
        void key(int chips, Efuse &efuse); //chips is the number of time segments (ASK bit times as defined by constants::bit_time) that you want to key a 1

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