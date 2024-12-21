/**
* @file PTT.cpp
* @author Tom Conrad (tom@silversat.org)
* @brief Library for a software based one-shot, used to drive the PTT pin on the Ground Station Amplifier
* @version 1.0.1
* @date 2024-12-10

PTT.cpp - Library for a software based one-shot, used to drive the PTT pin on the Ground Station Amplifier
Created by Tom Conrad, December 10, 2024.
Released into the public domain.

The PTT line will be driven high on the first transmission, with a fixed timeout defined by constants::PTT_duration
It will delay going high by constants::PTT_delay
It can be retriggered by a subsequent call
*/

#include "PTT.h"

PushToTalk::PushToTalk(int PTT_pin) 
{
  _pin = PTT_pin;
}

void PushToTalk::begin() 
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

//trigger or retrigger the timer
void PushToTalk::trigger(bool &PTT_flag) 
{
#ifdef SILVERSAT_GROUND
    if (PTT_flag == false)  //if the flag is low, then delay before continuing
    {
        digitalWrite(_pin, HIGH);  //set the ptt pin high
        delay(constants::PTT_delay);  //delay
    }
    Log.notice("PTT triggered\r\n");
    m_first_trigger = millis();  //store the trigger time
    Log.notice("trigger time: %i \r\n", m_first_trigger);
    PTT_flag = true; //set the PTT flag
#endif
};


//check the timer: for polling loop.
bool PushToTalk::check_timer(bool &PTT_flag) 
{
#ifdef SILVERSAT_GROUND
    //Log.notice("duration timer: %i\r\n", millis() - m_first_trigger);
    if ((millis() - m_first_trigger) > constants::PTT_duration)
    {   
        PTT_flag = false;  //reset the PTT flag
        digitalWrite(_pin, LOW);  //change the pin state to low
        return false;  //timer has expired
    }
    else
    {
        return true;  //timer is still active
    }
#else
    return false;
#endif

}