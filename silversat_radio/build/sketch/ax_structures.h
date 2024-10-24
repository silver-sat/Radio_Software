#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\ax_structures.h"
/**
 * @file ax_structures.h
 * @author Richard Meadows <richardeoin>
 * @author Thomas Conrad <tconrad@arcaneideas.com>
 * @brief Functions for controlling ax radio
 * @version 1.0
 * @date 2024
 *
 * Structures used in AX Library Functions for controlling ax radios
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
 *
 * _______________________________
 * This file is not part of the original AX library.  It was added to remove the circular
 * dependency between ax.h, ax_params.h and ax_hw.h.
 *
 * I moved the common structures into this file and then included it in ax.h,
 * ax_params.h and ax_hw.h.
 *
 * This allowed me to remove the #includes from ax.cpp (breaking the circular dependency)
 *
 * Tom Conrad 8/12/24
 *
 */
#pragma once

#ifndef AX_STRUCTURES_H
#define AX_STRUCTURES_H

#include "Arduino.h"

/**
 * Represents a receive parameter set for an ax5243 radio
 */
typedef struct ax_rx_param_set
{
    uint8_t agc_attack, agc_decay;
    uint32_t time_gain, dr_gain;
    uint8_t phase_gain, filter_idx;
    uint8_t baseband_rg_phase_det;
    uint8_t baseband_rg_freq_det;
    uint8_t rffreq_rg_phase_det;
    uint8_t rffreq_rg_freq_det;
    uint8_t amplgain, amplflags;
    uint16_t freq_dev;
} ax_rx_param_set;

/**
 * Represents the tweakable parameters for an ax5243 radio
 */
typedef struct ax_params
{
    uint8_t is_params_set; /* has this structure been set? */
    float m;               // modulation index

    // 5.6 forward error correction
    uint8_t fec_inp_shift;
    uint8_t shortmem;

    // 5.15 receiver parameters
    uint32_t rx_bandwidth;
    uint32_t f_baseband;
    uint32_t if_frequency;
    uint32_t iffreq;
    uint32_t decimation;
    uint32_t rx_data_rate;
    uint32_t max_rf_offset;
    uint32_t fskd;
    uint8_t ampl_filter;

    // 5.15.10 afskctrl
    uint8_t afskshift;

    // 5.15.15+ receiver parameter sets
    ax_rx_param_set rx_param_sets[4];

    // 5.20 packet format
    uint8_t fec_sync_dis;

    // 5.21 match parameters
    uint8_t match1_threashold;
    uint8_t match0_threashold;

    // 5.22 packet controller
    uint8_t pkt_misc_flags;
    uint16_t tx_pll_boost_time, tx_pll_settle_time;
    uint16_t rx_pll_boost_time, rx_pll_settle_time;
    uint16_t rx_coarse_agc;
    uint16_t rx_agc_settling, rx_rssi_settling;
    uint16_t preamble_1_timeout, preamble_2_timeout;
    uint8_t rssi_abs_thr;

    // 5.26 'performance tuning'
    uint8_t perftuning_option;

} ax_params;

/**
 * Represents the chosen modulation scheme.
 */
typedef struct ax_modulation
{
    uint8_t modulation; /* modulation */
    uint8_t encoding;   /* encoding */
    uint8_t framing;    /* framing */
    uint8_t shaping;    /* shaping */
    uint32_t bitrate;   /* symbol bitrate provided to user */
    uint8_t fec;        /* 0 = no fec, 1 = fec enabled */
    uint8_t radiolab;   /* 0 = use calculated values, 1 = use radiolab values*/
    uint8_t il2p_enabled; /* 0 = normal framing, 1 = il2p enabled*/

    float power; /* TX output power, as fraction of maximum */
    /* Pre-distortion is possible in hardware, but not supported here. */

    uint8_t continuous; /* 0 = occasional packets, 1 = continuous tx */

    uint8_t fixed_packet_length; /* 0 = variable length, 1-255 = length */

    union
    {
        struct
        { /* FSK */
            float modulation_index;
        } fsk;
        struct
        {                         /* AFSK */
            uint16_t deviation;   /* (Hz) */
            uint16_t space, mark; /* (Hz) */
        } afsk;
    } parameters;

    uint32_t max_delta_carrier; /* max. delta from carrier centre, autoset if 0 */
    /* larger increases the time for the AFC to achieve lock */

    ax_params par; /* tweakable parameters */

} ax_modulation;

/**
 * CONFIG ----------------------------------------------------------------------
 */

/* Clock source type */
enum ax_clock_source_type
{
    AX_CLOCK_SOURCE_CRYSTAL,
    AX_CLOCK_SOURCE_TCXO,
};

/* VCO type - See Datasheet Table 8. */
enum ax_vco_type
{
    AX_VCO_INTERNAL = 0,
    AX_VCO_INTERNAL_EXTERNAL_INDUCTOR,
    AX_VCO_EXTERNAL,
};

/* Divider at the output of the VCO  */
enum ax_rfdiv
{
    AX_RFDIV_UKNOWN = 0,
    AX_RFDIV_0,
    AX_RFDIV_1,
};

/* Transmit path */
enum ax_transmit_path
{
    AX_TRANSMIT_PATH_DIFF = 0,
    AX_TRANSMIT_PATH_SE,
};

/**
 * Represents one of the two physical synthesisers.
 */
typedef struct ax_synthesiser
{
    uint32_t frequency;
    uint32_t register_value;
    enum ax_rfdiv rfdiv; /* set if this is known, else it's set automatically */
    uint32_t frequency_when_last_ranged;
    uint8_t vco_range_known; /* set to 0 if vco range unknown */
    uint8_t vco_range;       /* determined by autoranging */
} ax_synthesiser;

/**
 * configuration
 */
typedef struct ax_config
{
    /* power mode */
    uint8_t pwrmode;

    /* synthesiser */
    struct
    {
        ax_synthesiser A, B;
        enum ax_vco_type vco_type; /* default is internal */
    } synthesiser;

    /* external clock */
    enum ax_clock_source_type clock_source; /* Crystal or TCXO */
    uint32_t f_xtal;                        /* external clock frequency (Hz) */
    uint16_t load_capacitance;              /* if crystal, load capacitance to be applied (pF) */
    uint32_t error_ppm;                     /* max. error of clock source, ppm */
    uint8_t f_xtaldiv;                      /* xtal division factor, set automatically */
    void *(*tcxo_enable)(void);             /* function to enable tcxo */
    void *(*tcxo_disable)(void);            /* function to disable tcxo */

    /* transmit path */
    enum ax_transmit_path transmit_path; /* transmit path */
    float transmit_power_limit;          /* power limit */

    /* spi transfer */
    void (*spi_transfer)(unsigned char *, uint8_t);

    /* receive */
    uint8_t pkt_store_flags;  /* PKTSTOREFLAGS */
    uint8_t pkt_accept_flags; /* PKTACCEPTFLAGS */
    /* Note that we always accept multiple chunks (LRGP), bad address
     * (ADDRF), and nonintegral number of bytes in HDLC (RESIDUE) */

    /* wakeup */
    uint32_t wakeup_period_ms;
    uint32_t wakeup_xo_early_ms;

    /* digitial to analogue (DAC) channel */
    uint8_t dac_config;

    /* pll vco */
    uint32_t f_pllrng;

} ax_config;

#endif