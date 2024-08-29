#line 1 "C:\\Users\\conra\\OneDrive\\Documents\\GitHub\\Radio_Software\\e2ecommands_metro\\antenna.cpp"
/**
* @file antenna.cpp
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing antenna support for the Silversat Radio
* @version 1.0.1
* @date 2023-7-17

antenna.cpp - Library providing antenna support for the Silversat Radio
Created by Tom Conrad, July 17, 2024.
Released into the public domain.

*/

#include "antenna.h"


Antenna::Antenna(int Release_A_pin, int Release_B_pin)
{
    _release_A_pin = Release_A_pin;
    _release_B_pin = Release_B_pin;
}

void Antenna::begin()
{
    pinMode(_release_A_pin, OUTPUT);    //for Endurosat antenna
    pinMode(_release_B_pin, OUTPUT);    //for Endurosat antenna
    _release_timer_start = millis();
}

void Antenna::release(char select, ExternalWatchdog &watchdog, String &response)
{
    if (select == 0x43)
    {
        release_AB(watchdog);
        debug_printf("Both cycles complete \r\n");
        response = "";
    }

    else if (select == 0x42)
    {
        release_B(watchdog);
        debug_printf("Release_B cycle complete \r\n");
        response = "";
    }

    else if (select == 0x41)
    {
        release_A(watchdog);
        debug_printf("Release_A cycle complete \r\n");
        response = "";
    }
}

void Antenna::release_A(ExternalWatchdog &watchdog)
{
    digitalWrite(Release_A, 1);
    digitalWrite(Release_B, 0);
    while (millis() - _release_timer_start < 30000)
    {
        watchdog.trigger();
    }
    digitalWrite(Release_A, 0);
    digitalWrite(Release_B, 0);
}

void Antenna::release_B(ExternalWatchdog &watchdog)
{
    digitalWrite(Release_A, 0);
    digitalWrite(Release_B, 1);
    while (millis() - _release_timer_start < 30000)
    {
        watchdog.trigger();
    }
    digitalWrite(Release_A, 0);
    digitalWrite(Release_B, 0);
}

void Antenna::release_AB(ExternalWatchdog &watchdog)
{
    digitalWrite(Release_A, 1);
    digitalWrite(Release_B, 1);
    while (millis() - _release_timer_start < 30000)
    {
        watchdog.trigger();
    }
    digitalWrite(Release_A, 0);
    digitalWrite(Release_B, 0);
}