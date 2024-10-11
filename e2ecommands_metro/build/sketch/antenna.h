#line 1 "C:\\Users\\conra\\OneDrive\\Documents\\GitHub\\Radio_Software\\e2ecommands_metro\\antenna.h"
/**
* @file antenna.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing antenna support for the Silversat Radio
* @version 1.0.1
* @date 2023-7-17

antenna.h - Library providing antenna support for the Silversat Radio
Created by Tom Conrad, July 17, 2024.
Released into the public domain.


*/

#ifndef ANTENNA_H
#define ANTENNA_H

#include "constants.h"
#include "ExternalWatchdog.h"

#include <LibPrintf.h>
#include <ArduinoLog.h>

//#include <Arduino.h>

class Antenna
{
public:
    Antenna(int Release_A_pin, int Release_B_pin);
    void begin();
    void release(char select, ExternalWatchdog &watchdog, String &response);

private:
    int _release_A_pin;
    int _release_B_pin;
    int _release_timer_start;
    int m_last_action_time;

    void release_A(ExternalWatchdog &watchdog);
    void release_B(ExternalWatchdog &watchdog);
    void release_AB(ExternalWatchdog &watchdog);
};

#endif