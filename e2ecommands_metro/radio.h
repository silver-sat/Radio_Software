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

#include "ax.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "constants.h"
#include "efuse.h"
#include "ExternalWatchdog.h"
#include <Temperature_LM75_Derived.h>
#include <FlashStorage.h>
#include <ArduinoLog.h>

//#include "Arduino.h"

class Radio
{
public:
  Radio(int TX_RX_pin, int RX_TX_pin, int PA_enable_pin, int SYSCLK_pin, int AX5043_DCLK_pin, int AX5043_DATA_pin, int PIN_LED_TX_pin, int IRQ_pin);

  void begin(void (*spi_transfer)(unsigned char *, uint8_t), FlashStorageClass<int> &operating_frequency);
  
  void beaconMode();  //ASK mode to send out the satellite beacon
  void key(int chips, Efuse &efuse); // chips is the number of time segments (ASK bit times as defined by constants::bit_time) that you want to key a 1
  //ideas below this point

  void dataMode();  //(G)FSK mode to send out data.  NOTE: Modulation type choice and error correction state set by writing to radio.modulation structure
  void setTransmit(); //selects transmit or receive state
  void setReceive();
  int setTransmitFrequency(int frequency);
  int setReceiveFrequency(int frequency);
  void transmit(byte* txqueue, int txbufflen);
  bool receive();

  int getTransmitFrequency();
  int getReceiveFrequency();

  void cwMode(uint32_t duration, ExternalWatchdog &watchdog);  //used for testing
  
  //misc utility functions
  size_t reportstatus(String &response, Efuse &efuse, bool fault);
  bool radioBusy();
  uint8_t rssi();
  void clear_Radio_FIFO();
  uint16_t getRegValue(int register);
  
  //these are all used by the AX library so have to remain public
  ax_packet rx_pkt; // instance of packet structure
  ax_config config; // radio config
  ax_modulation modulation; // modulation structure

  //synthesizer functions
  void setSynthA(); //directly set the Tx synth to be active
  void setSynthB(); //directly set the Rx synth to be active
  uint8_t getSynth();  //returns which synth is selected.  0 for Tx, 1 for Rx

  
private:
  int _pin_TX_RX;
  int _pin_RX_TX;
  int _pin_PAENABLE;
  int _pin_SYSCLK;
  int _pin_AX5043_DCLK;
  int _pin_AX5043_DATA;
  int _pin_TX_LED;
  pinfunc_t _func{2}; // definition of wire vs data mode
  void ISR();


};

#endif