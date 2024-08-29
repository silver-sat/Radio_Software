/**
* @file efuse.cpp
* @author Tom Conrad (tom@silversat.org)
* @brief Library for using the eFuse, specifically a TPS25940LQRVCRQ1
* @version 1.0.1
* @date 2024-7-2

eFuse.cpp - Library for using the eFuse, specifically a TPS25940LQRVCRQ1
Created by Tom Conrad, July 2, 2024.
Released into the public domain.

*/

#include "efuse.h"


Efuse::Efuse(int I_Monitor_pin, int OC5V_pin, int Efuse_reset_pin)
{
    _pin_5V_current = I_Monitor_pin;
    _pin_OC5V = OC5V_pin;
    _pin_5V_reset = Efuse_reset_pin;
    _imon_intercept = 42.64; //millivolts
    _imon_slope = 2786; //millivolts, current in amperes
    _adc_resolution = 3.3 / 1024;
    _OC_threshold_transmit = 600;
    _OC_threshold_receive = 50;
    m_repeat_timer = millis();
}

void Efuse::begin()
{
    //pinMode(_pin_5V_current, INPUT);
    pinMode(Efuse::_pin_OC5V, INPUT);
    pinMode(Efuse::_pin_5V_reset, INPUT);
}

float Efuse::measure_current()
{
    int imon_reading = analogRead(Efuse::_pin_5V_current);
    float imon_voltage = imon_reading * _adc_resolution * 1000; //reading * resolution in volts/count * 1000 to convert to millivolts
    float current = (imon_voltage - _imon_intercept)/_imon_slope * 1000;
    if (current > _max_current)
    {
        _max_current = current;
    }
    return current;
}

bool Efuse::fault()
{
    return !digitalRead(_pin_OC5V);  //pin is active low, changing to make a fault return true.
}

int Efuse::overcurrent(bool transmit)
{
    //set threshold
    float _threshold = _OC_threshold_receive;
    if (transmit == true)
    {
       _threshold = _OC_threshold_transmit;
    }

    //measure to see if there's an overcurrent and report
    int imon_reading = analogRead(_pin_5V_current);
    float imon_voltage = imon_reading * _adc_resolution * 1000;
    float current = (imon_voltage - _imon_intercept) / _imon_slope * 1000;
    // if the current is greater than the threshold and the timer has expired, or if the fault line is low, then return true
    if (((current > _threshold) && (millis()-m_repeat_timer > 500)) || !digitalRead(_pin_OC5V))
    {
        //m_repeat_timer is initialized when efuse.begin() is executed
        //we have an overcurrent, so send the packet and reset the timer, don't resend until timer value is greater than 500mS
        byte resetpacket[] = {0xC0, 0x0F, 0xC0}; // generic form of nack packet
    #ifdef SILVERSAT
        Serial0.write(resetpacket, 3);        
    #endif
        m_repeat_timer = millis();
        return 1;
    }
    else return 0;
}

void Efuse::reset()
{
    pinMode(_pin_5V_reset, OUTPUT);
    digitalWrite(_pin_5V_reset, LOW);
    delay(1);  //actual value TBD, need to see what works...typ 2 microseconds according to datasheet
    pinMode(_pin_5V_reset, INPUT);
}

void Efuse::set_OC_threshold(float threshold, bool transmit)
{
    if (transmit == true)
    {
        _OC_threshold_transmit = threshold;
    }
    else
    {
        _OC_threshold_receive = threshold;
    }
}

void Efuse::set_imon_slope(float imon_slope)
{
    _imon_slope = imon_slope;
}

void Efuse::set_imon_intercept(float imon_intercept)
{
    _imon_intercept = imon_intercept;
}

float Efuse::get_max_current()
{
    return _max_current;
}

void Efuse::clear_max_current()
{
    _max_current = 0;
}
