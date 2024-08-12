/*
 * Functions for accessing ax hardware using wiring pi
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

#ifndef AX_HW_H
#define AX_HW_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "ax.h"
#include "ax_reg.h"
#include "ax_fifo.h"

uint8_t ax_hw_read_register_long_8(ax_config* config, uint16_t reg);
uint16_t ax_hw_write_register_long_8(ax_config* config, uint16_t reg, uint8_t value);
uint8_t ax_hw_read_register_8(ax_config* config, uint16_t reg);
uint16_t ax_hw_write_register_8(ax_config* config, uint16_t reg, uint8_t value);

uint16_t ax_hw_read_register_long_bytes(ax_config* config, uint16_t reg,
                                        uint8_t* ptr, uint8_t bytes);
uint16_t ax_hw_read_register_bytes(ax_config* config, uint16_t reg,
                                   uint8_t* ptr, uint8_t bytes);

uint16_t ax_hw_write_register_16(ax_config* config, uint16_t reg, uint16_t value);
uint16_t ax_hw_write_register_24(ax_config* config, uint16_t reg, uint32_t value);
uint16_t ax_hw_write_register_32(ax_config* config, uint16_t reg, uint32_t value);
uint16_t ax_hw_read_register_16(ax_config* config, uint16_t reg);
uint32_t ax_hw_read_register_24(ax_config* config, uint16_t reg);
uint32_t ax_hw_read_register_32(ax_config* config, uint16_t reg);

uint16_t ax_hw_write_fifo(ax_config* config, uint8_t* buffer, uint16_t length);
uint16_t ax_hw_read_fifo(ax_config* config, uint8_t* buffer, uint16_t length);

uint16_t ax_hw_status(void);
uint16_t ax_hw_poll_status(void);

#endif  /* AX_HW_H */
