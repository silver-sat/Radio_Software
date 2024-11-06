/**
 * @file ax_modes.cpp
 * @author Richard Meadows <richardeoin>
 * @brief Example mode implementations for ax5243
 * @version 1.0
 * @date 2016
 *
 * Example mode implementations for ax5243
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

#include "ax_modes.h"

/**
 * Each struct represents a useful mode
 */

// GMSK test
struct ax_modulation gmsk_modulation = {
    .modulation = AX_MODULATION_FSK,
    .encoding = AX_ENC_NRZI,
    .framing = AX_FRAMING_MODE_HDLC | AX_FRAMING_CRCMODE_CCITT,
    .shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5,
    .bitrate = 9600,
    .fec = 0,
    .radiolab = 1,
    .il2p_enabled = 0, 
    .power = constants::power,
    .continuous = 0,
    .fixed_packet_length = 0,
    .parameters = {.fsk = {.modulation_index = 0.5}},
    .max_delta_carrier = 0,
    .par = {},
};

// GMSK HDLC FEC test
// NOTE: be sure to modify the preamble if you enable FEC!!!
/*
struct ax_modulation gmsk_hdlc_fec_modulation = {
  .modulation = AX_MODULATION_FSK,
  .encoding = AX_ENC_NRZ + AX_ENC_SCRAM,
  .framing = AX_FRAMING_MODE_HDLC | AX_FRAMING_CRCMODE_CCITT,
  .shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5,
  .bitrate = 19200,
  .fec = 1,
  .il2p_enabled = 0,
  .power = 1.0,
  .continuous = 0,
  .fixed_packet_length=0,
  .parameters = {.fsk = { .modulation_index = 0.5 }},
  .max_delta_carrier = 0,
  .par={},
};
*/

/* FSK */
struct ax_modulation fsk_modulation = {
    .modulation = AX_MODULATION_FSK,
    .encoding = AX_ENC_NRZI,
    //.encoding = AX_ENC_NRZ + AX_ENC_SCRAM,
    .framing = AX_FRAMING_MODE_HDLC | AX_FRAMING_CRCMODE_CCITT,
    .shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED,
    .bitrate = 1200,
    .fec = 0,
    .radiolab = 0,
    .il2p_enabled = 0,
    .power = constants::power,
    .continuous = 0,
    .fixed_packet_length = 0,
    .parameters = {.fsk = {.modulation_index = 2.0 / 3}},
    .max_delta_carrier = 0, // 0 sets it to the default, which is defined in constants.cpp
    .par = {},
};

/* ASK modulation */
struct ax_modulation ask_modulation = {
    .modulation = AX_MODULATION_ASK_COHERENT,
    .encoding = AX_ENC_NRZ,
    .framing = AX_FRAMING_CRCMODE_OFF,
    .shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED,
    .bitrate = 1000, // 100 bits per second or more importantly 1mSec per 1, sets time resolution
    .fec = 0,
    .radiolab = 1,
    .il2p_enabled = 0,
    .power = constants::power,
    .continuous = 1, // this more applies to the receiver, so doesn't really matter
    .fixed_packet_length = 0,
    .parameters = {},
    .max_delta_carrier = 0, // 0 sets it to the default, which is defined in constants.cpp
    .par = {.perftuning_option = 1},
};

/*
// CW method using FSK
struct ax_modulation fsk_cw_modulation = {
    .modulation = AX_MODULATION_FSK,
    .encoding = AX_ENC_NRZ,
    .framing = AX_FRAMING_CRCMODE_OFF,
    .shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED,
    .bitrate = 2, //method says almost 1, but 1 didn't work.  2 seems to because it looks like it writes 0x000001 into the bit rate register
    .fec = 0,
    .il2p_enabled = 0,
    .power = 1,
    .continuous = 1,
    .fixed_packet_length=0,
    .parameters = { .fsk = { .modulation_index = 0 }},
    .max_delta_carrier = 0, //0 sets it to the default, which is defined in constants.cpp
    .par={},
    };
  */

  /* APRS */
struct ax_modulation aprs_modulation = {
  .modulation = AX_MODULATION_AFSK,
  .encoding = AX_ENC_NRZI,
  .framing = AX_FRAMING_MODE_HDLC | AX_FRAMING_CRCMODE_CCITT,
  .shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED,
  .bitrate = 1200,
  .fec = 0,
  .radiolab = 1,
  .il2p_enabled = 0, 
  .power = constants::power,
  .continuous = 0,
  .fixed_packet_length = 0,
  .parameters = { .afsk = {
      .deviation = 3000, .space = 2200, .mark = 1200,  }},
  .max_delta_carrier = 0, // 0 sets it to the default, which is defined in constants.cpp
  .par = {},
};

// GMSK raw packets
struct ax_modulation gmsk_modulation_raw = {
    .modulation = AX_MODULATION_FSK,
    .encoding = AX_ENC_NRZ,
    .framing = AX_FRAMING_MODE_RAW_PATTERN_MATCH | AX_FRAMING_CRCMODE_CCITT,
    .shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5,
    .bitrate = 9600,
    .fec = 0,
    .radiolab = 1,
    .il2p_enabled = 0,
    .power = constants::power,
    .continuous = 0,
    .fixed_packet_length = 0,
    .parameters = {.fsk = {.modulation_index = 0.5}},
    .max_delta_carrier = 0,
    .par = {},
};

// GMSK raw packets with IL2P at 9600
struct ax_modulation gmsk_modulation_il2p = {
    .modulation = AX_MODULATION_FSK,
    .encoding = AX_ENC_NRZ,
    .framing = AX_FRAMING_MODE_RAW_PATTERN_MATCH | AX_FRAMING_CRCMODE_OFF,
    .shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5,
    .bitrate = 9600,
    .fec = 0,
    .radiolab = 1,
    .il2p_enabled = 1,
    .power = constants::power,
    .continuous = 0,
    .fixed_packet_length = 0,
    .parameters = {.fsk = {.modulation_index = 0.5}},
    .max_delta_carrier = 0,
    .par = {},
};

// GMSK raw packets with IL2P at 4800
struct ax_modulation gmsk_modulation_il2p_4800 = {
    .modulation = AX_MODULATION_FSK,
    .encoding = AX_ENC_NRZ,  // setting invert here does not invert the data
    .framing = AX_FRAMING_MODE_RAW_PATTERN_MATCH | AX_FRAMING_CRCMODE_OFF,
    .shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5,
    .bitrate = 4800,
    .fec = 0,
    .radiolab = 1,
    .il2p_enabled = 1,
    .power = constants::power,
    .continuous = 0,
    .fixed_packet_length = 0,
    .parameters = {.fsk = {.modulation_index = 0.5}},
    .max_delta_carrier = 0,
    .par = {},
};