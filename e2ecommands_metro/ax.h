/**
 * @file ax.h
 * @author Richard Meadows <richardeoin>
 * @brief Functions for controlling ax radio
 * @version 1.0
 * @date 2016
 *
 * Functions for controlling ax radios
 * Copyright (C) 2016  Richard Meadows <richardeoin>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef AX_H
#define AX_H

//#define DEBUG

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


#ifdef SILVERSAT
#define _AX_TX_SE
#endif

#ifndef SILVERSAT
#define _AX_TX_DIFF
#endif

#define USE_MATH_H
#ifdef USE_MATH_H
#include <math.h>
#endif

#include <LibPrintf.h>
#include "fec.h"

#include "ax_structures.h"
#include "ax_reg.h"
#include "ax_reg_values.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_params.h"
//#include "ax_modes.h"

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

/* synthesizer loop parameters */
typedef struct ax_synthesiser_parameters
{
    uint8_t loop, charge_pump_current;
} ax_synthesiser_parameters;

/**
 * Initialisation Status
 */
typedef enum ax_init_status {
  AX_INIT_OK = 0,
  AX_INIT_PORT_FAILED,
  AX_INIT_BAD_SCRATCH,
  AX_INIT_BAD_REVISION,
  AX_INIT_SET_SPI,
  AX_INIT_VCO_RANGING_FAILED,
} ax_init_status;

/**
 * pin functions
 */
typedef uint8_t pinfunc_t;

/**
 * Represents a received packet and sets the size of the data buffer at 512 bytes - remember that an ax packet can span more than one radio packet using the extension flags
 */
#define AX_PACKET_MAX_DATA_LENGTH	0x200

typedef struct ax_packet {
  unsigned char data[0x200];
  uint16_t length;
  int16_t rssi;
  int32_t rffreqoffs;
} ax_packet;

/**
 * configuration for wakeup
 */
typedef struct ax_wakeup_config {
  uint32_t wakeup_period_ms;    /* period of wakeups, in ms */
  uint32_t wakeup_xo_early_ms;  /* wakeup the XO before, in ms */

  uint32_t wakeup_duration_bits; /* length the wakeup event if no packet is rx'd */
  /* suggest 25 */

  uint8_t rssi_abs_thr;         /* rssi threashold for wakeup event to happen  */
  /* suggest 221, or 3log2(b/w) + x  */
} ax_wakeup_config;

enum ax_vco_ranging_result
{
    AX_VCO_RANGING_SUCCESS,
    AX_VCO_RANGING_FAILED,
};

/**
 * FUNCTION PROTOTYPES ---------------------------------------------------------
 */

// private function prototypes
void ax_set_synthesiser_parameters(ax_config *config, ax_synthesiser_parameters *params, ax_synthesiser *synth, enum ax_vco_type vco_type);
void ax_fifo_commit(ax_config *config);
void ax_fifo_tx_1k_zeros(ax_config *config);
void ax_wait_for_oscillator(ax_config *config);
static uint8_t ax_value_to_mantissa_exp_4_4(uint32_t value);
static uint8_t ax_value_to_exp_mantissa_3_5(uint32_t value);
//set or query registers
uint8_t ax_silicon_revision(ax_config *config);
uint8_t ax_scratch(ax_config *config);
void ax_set_modulation_parameters(ax_config *config, ax_modulation *mod);
void ax_set_pin_configuration(ax_config *config);
uint32_t ax_set_freq_register(ax_config *config, uint8_t reg, uint32_t frequency);
void ax_set_synthesiser_frequencies(ax_config *config);
void ax_set_performance_tuning(ax_config *config, ax_modulation *mod);
void ax_set_wakeup_timer(ax_config *config, ax_wakeup_config *wakeup_config);
void ax_set_afsk_rx_parameters(ax_config *config, ax_modulation *mod);
void ax_set_rx_parameters(ax_config *config, ax_modulation *mod);
void ax_set_rx_parameter_set(ax_config *config, uint16_t ps, ax_rx_param_set *pars);
void ax_set_afsk_tx_parameters(ax_config *config, ax_modulation *mod);
uint8_t ax_modcfga_tx_parameters_tx_path(enum ax_transmit_path path);
void ax_set_tx_parameters(ax_config *config, ax_modulation *mod);
void ax_set_pll_parameters(ax_config *config);
void ax_set_xtal_parameters(ax_config *config);
void ax_set_baseband_parameters(ax_config *config);
void ax_set_packet_parameters(ax_config *config, ax_modulation *mod);
void ax_set_pattern_match_parameters(ax_config *config, ax_modulation *mod);
void ax_set_packet_controller_parameters(ax_config *config, ax_modulation *mod, ax_wakeup_config *wakeup_config);
void ax_set_low_power_osc(ax_config *config, ax_wakeup_config *wakeup_config);
void ax_set_digital_to_analog_converter(ax_config *config);
void ax_set_registers(ax_config *config, ax_modulation *mod, ax_wakeup_config *wakeup_config);
void ax_set_registers_tx(ax_config *config, ax_modulation *mod);
void ax_set_registers_rx(ax_config *config, ax_modulation *mod);
//vco functions
enum ax_vco_ranging_result ax_do_vco_ranging(ax_config *config, uint16_t pllranging, ax_synthesiser *synth, enum ax_vco_type vco_type);
enum ax_vco_ranging_result ax_vco_ranging(ax_config *config);

/* tweakable parameters */
void ax_default_params(ax_config *config, ax_modulation *mod);

/* adjust frequency */
int ax_adjust_frequency_A(ax_config* config, uint32_t frequency);
int ax_adjust_frequency_B(ax_config *config, uint32_t frequency);
int ax_force_quick_adjust_frequency_A(ax_config* config, uint32_t frequency);
int ax_force_quick_adjust_frequency_B(ax_config *config, uint32_t frequency);

/* transmit */
void ax_tx_on(ax_config* config, ax_modulation* mod);
void ax_tx_packet(ax_config* config, ax_modulation* mod,
                  uint8_t* packet, uint16_t length);
void ax_tx_beacon(ax_config* config,
                  uint8_t* packet, uint16_t length);
void ax_tx_1k_zeros(ax_config* config);
void ax_fifo_tx_beacon(ax_config* config,
                  uint8_t* data, uint16_t length);
void ax_fifo_tx_data(ax_config* config, ax_modulation* mod,
                  uint8_t* data, uint16_t length);
                  
/* FIFO */
void ax_fifo_clear(ax_config* config);
                 

/* receive */
void ax_rx_on(ax_config* config, ax_modulation* mod);
void ax_rx_wor(ax_config* config, ax_modulation* mod,
               ax_wakeup_config* wakeup_config);
int ax_rx_packet(ax_config* config, ax_packet* rx_pkt, ax_modulation* modulation);
uint16_t ax_fifo_rx_data(ax_config *config, ax_rx_chunk *chunk);

/* turn off */
void ax_off(ax_config *config);
void ax_force_off(ax_config* config);

/* pinfunc */
void ax_set_pinfunc_sysclk(ax_config* config, pinfunc_t func);
void ax_set_pinfunc_dclk(ax_config* config, pinfunc_t func);
void ax_set_pinfunc_data(ax_config* config, pinfunc_t func);
void ax_set_pinfunc_antsel(ax_config* config, pinfunc_t func);
void ax_set_pinfunc_pwramp(ax_config* config, pinfunc_t func);

/* tx path */
void ax_set_tx_path(ax_config* config, enum ax_transmit_path path);

/* init */
int ax_init(ax_config* config);

/* RSSI - used for CCA */
uint8_t ax_RSSI(ax_config* config);

/* BGNDRSSI - used for CCA */
uint8_t ax_BGNDRSSI(ax_config* config);

/* RADIOSTATE - used to determine if transmit is complete */
uint8_t ax_RADIOSTATE(ax_config* config);

/* POWSTAT - used to query the power status register */
uint8_t ax_POWSTAT(ax_config* config);

/* POWSTICKYSTAT - used to query the power sticky status register */
uint8_t ax_POWSTICKYSTAT(ax_config* config);

/* FIFOSTAT - used to query the status of the fifo */
uint8_t ax_FIFOSTAT(ax_config* config);

/* FIFODATA - gives the fifo address pointer? */
uint8_t ax_FIFODATA(ax_config* config);

/* FIFOCOUNT - returns the amount of data in the FIFO */
uint16_t ax_FIFOCOUNT(ax_config* config);

/* FIFOFREE - returns the amount of free space in the FIFO */
uint16_t ax_FIFOFREE(ax_config* config);

/* TXPWRCOEFFB - returns the value of the TXPWRCOEFFB register, related to output power */
uint16_t ax_TXPWRCOEFFB(ax_config* config);

/* PWRMODE - sets the powermode, needed to change from Rx to TX according to errata; this command was meant to be private, so be careful*/
void ax_set_pwrmode(ax_config* config, uint8_t pwrmode);

/* MODIFY_TX_POWER - modifies the output power between 0.1 and 1, esentially +5 to 15dBm theoretically*/
/* it's actually a sixteen bit value assuming there's no pre-distortion.  You're adjusting TXPWRCOEFFB */
uint16_t ax_MODIFY_TX_POWER(ax_config* config, float new_power);

/* MODIFY_FEC enables or disables FEC.  Changes value in structure.  New mode still must be loaded.*/
uint16_t ax_MODIFY_FEC(ax_config* config, ax_modulation* current_mod, bool FEC);

/* MODIFY_SHAPING configures frequency shaping profile.  Changes value in structure.  New mode still must be loaded.*/
uint16_t ax_MODIFY_SHAPING(ax_config* config, ax_modulation* current_mod, uint8_t shaping);

/* ax_TOGGLE_SYNTH switches between the FREQA and FREQB registers*/
uint8_t ax_TOGGLE_SYNTH(ax_config *config);

uint8_t ax_SET_SYNTH_A(ax_config *config);

uint8_t ax_SET_SYNTH_B(ax_config *config);

#endif  /* AX_H */
