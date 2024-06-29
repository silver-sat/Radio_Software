/*
  ExternalWatchdog.h - Library for using the external watchdog, specifically a TPS3813K33-Q1
  Created by Tom Conrad, June 29, 2024.
  Released into the public domain.

*/

#include "Arduino.h"

#include "ExternalWatchdog.h"


ExternalWatchdog::ExternalWatchdog(int WDTICK_pin)
{
  _pin = WDTICK_pin;

}


void ExternalWatchdog::begin()
{
  pinMode(_pin, OUTPUT);
  m_last_action_time = millis();
}


void ExternalWatchdog::trigger()
{
    if (millis() - m_last_action_time > watchdog_lower_boundary)
    {
        digitalWrite(_pin, HIGH);
        digitalWrite(_pin, LOW);
        m_last_action_time = millis();
    };
};