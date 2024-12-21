/**
* @file PTT.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library for a software based one-shot, used to drive the PTT pin on the Ground Station Amplifier
* @version 1.0.1
* @date 2024-12-10

PTT.h - Library for a software based one-shot, used to drive the PTT pin on the Ground Station Amplifier
Created by Tom Conrad, December 10, 2024.
Released into the public domain.

The PTT line will be driven high on the first transmission, with a fixed timeout defined by constants::PTT_duration
It will delay going high by constants::PTT_delay
It can be retriggered by a subsequent call
*/

#ifndef PTT_H
#define PTT_H

#include "Arduino.h"
#include "constants.h"
#include "ArduinoLog.h"

class PushToTalk {
public:
  PushToTalk(int PTT_pin);

  void begin();
  void trigger(bool &PTT_flag);
  bool check_timer(bool &PTT_flag);

private:
  int _pin;
  int m_first_trigger;
};

#endif