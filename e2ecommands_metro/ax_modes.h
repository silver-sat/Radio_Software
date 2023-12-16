/*
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
#pragma once

#ifndef AX_MODES
#define AX_MODES

#include "ax.h"

// pared down to a small subset.  gmsk types are under test, fsk and ask have been used so far.

/* GMSK */
extern struct ax_modulation gmsk_modulation;
/* GMSK HDLC FEC */
extern struct ax_modulation gmsk_hdlc_fec_modulation;
/* FSK */
extern struct ax_modulation fsk_modulation;
/* ask modulation (beacon) */
extern struct ax_modulation ask_modulation;
/* CW using FSK and small offset - not currently used */
extern struct ax_modulation fsk_cw_modulation;

#endif /* AX_MODES */
