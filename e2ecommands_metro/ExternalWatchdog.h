/*
  ExternalWatchdog.h - Library for using the external watchdog, specifically a TPS3813K33-Q1
  Created by Tom Conrad, June 29, 2024.
  Released into the public domain.

  This device has two key parameters: t_boundary and t_window, which are defined by the pin configuration
  For the silversat radio board, WDT=VCC, WDR=VCC, which defines t_boundary(max) = 23.5mS, and t_window(min) = 2 Seconds
  A watchdog tick must be issued between t_boundary and t_window, or the system will reset.
*/

#ifndef EXTERNALWATCHDOG_H
#define EXTERNALWATCHDOG_H

#include "Arduino.h"

class ExternalWatchdog
{
  public:
    ExternalWatchdog(int WDTICK_pin);

    void begin();
    void trigger();

  private:
    int _pin;
    int watchdog_lower_boundary {24};
    int m_last_action_time;
};

#endif