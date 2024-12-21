/**
* @file ExternalWatchdog.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library for using the external watchdog, specifically a TPS3813K33-Q1
* @version 1.0.1
* @date 2024-6-29

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
#include "ArduinoLog.h"

class ExternalWatchdog {
public:
  ExternalWatchdog(int WDTICK_pin);

  void begin();
  void trigger();

private:
  int _pin;
  unsigned long watchdog_lower_boundary{ 24 };
  unsigned long m_last_action_time;
};

#endif