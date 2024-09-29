/**
 * @file ax_params.h
 * @author Richard Meadows <richardeoin>
 * @brief Calcuates tweakable parameters for an ax radio
 * @version 1.0
 * @date 2016
 *
 * Calcuates tweakable parameters for an ax radio
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

#ifndef AX_PARAMS_H
#define AX_PARAMS_H

#include "ax_structures.h"
#include "ax_reg_values.h"
#include "constants.h"

#include <LibPrintf.h>
#include <ArduinoLog.h>

#define USE_MATH_H
#ifdef USE_MATH_H
#include <math.h>
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* populates ax_params structure */
void ax_populate_params(ax_config *config, ax_modulation *mod, ax_params *par);

#endif /* AX_PARAMS_H */
