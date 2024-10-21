/**
* @file efuse.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library for using the eFuse, specifically a TPS25940LQRVCRQ1
* @version 1.0.1
* @date 2024-7-2

eFuse.h - Library for using the eFuse, specifically a TPS25940LQRVCRQ1
Created by Tom Conrad, July 2, 2024.
Released into the public domain.

The TPS25940LQRVCRQ1 eFuse has two main connections to the Silversat processor
via the 5V_Current and the OC5V signals.

Currently we do not have a direct way to reset the eFuse.  Reset is done by cycling the input voltage,
and that is accomplished by asking Avionics to instruct the EPS to cycle the 5V power.

The 5V_current signal is relatively linear.  We use that signal to indicate that the overcurrent threshold has been reached.
The OC5V signal is not as reliable indication of an overcurrent as we would have hoped.  A fault is only asserted (active low) in certain conditions.
When the output is shorted, it will limit the current to the designed amount.  It also drops the output voltage to a small value (measured 8mV),
but continues to source current.
The Pgood signal indicates if the output is below the Pgood threshold.

We can use a spare IO line to disable and re-enable the eFuse.
This is far faster and removes the Avionics board as the primary means to cycle the supply.
On the processor, the 5V reset signal is normally set as an INPUT.  If an overcurrent is detected, then it is reconfigured as an OUTPUT, and set to ground.
After a small timeout, it is release by again redefining the signal as an INPUT

*/

#ifndef EFUSE_H
#define EFUSE_H

#include "Arduino.h"
#include "ArduinoLog.h"


class Efuse
{
public:
    Efuse(int I_Monitor_pin, int OC5V_pin, int Efuse_reset_pin);

    void begin();
    int measure_current();
    int overcurrent(bool transmit);
    bool fault();
    void reset();
    void set_OC_threshold(float threshold, bool transmit);
    void set_imon_slope(float imon_slope);
    void set_imon_intercept(float imon_intercept);
    int get_max_current();
    void clear_max_current();

private:
    int _pin_5V_current;
    int _pin_OC5V;
    int _pin_5V_reset;
    int _pin_pgood;
    float _imon_slope;
    float _imon_intercept;
    float _OC_threshold_transmit;
    float _OC_threshold_receive;
    float _adc_resolution;
    int m_repeat_timer;
    int _max_current;
};

#endif