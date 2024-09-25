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

#define DEBUG


#include "ax.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "constants.h"
#include "efuse.h"
#include "ExternalWatchdog.h"

#include <Temperature_LM75_Derived.h>
#include <LibPrintf.h>
#include <FlashStorage.h>

//#include "Arduino.h"

class Radio
{
public:
  Radio(int TX_RX_pin, int RX_TX_pin, int PA_enable_pin, int SYSCLK_pin, int AX5043_DCLK_pin, int AX5043_DATA_pin, int PIN_LED_TX_pin);

  void begin(void (*spi_transfer)(unsigned char *, uint8_t), FlashStorageClass<int> &operating_frequency);
  void setTransmit();
  void setReceive();
  int setTransmitFrequency(int frequency);
  int setReceiveFrequency(int frequency);
  void beaconMode();
  void dataMode();
  void cwMode(uint32_t duration, ExternalWatchdog &watchdog);
  size_t reportstatus(String &response, Efuse &efuse, bool fault);
  void key(int chips, Efuse &efuse); // chips is the number of time segments (ASK bit times as defined by constants::bit_time) that you want to key a 1
  //ideas below this point
  bool radioBusy();
  int rssi();
  void transmit(byte* txqueue, int txbufflen);
  bool receive();
  ax_packet rx_pkt; // instance of packet structure
  ax_config config; // radio config
  ax_modulation modulation; // modulation structure

private:
  int _pin_TX_RX;
  int _pin_RX_TX;
  int _pin_PAENABLE;
  int _pin_SYSCLK;
  int _pin_AX5043_DCLK;
  int _pin_AX5043_DATA;
  int _pin_TX_LED;
  pinfunc_t _func{2}; // definition of wire vs data mode


};

#endif