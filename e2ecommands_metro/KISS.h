/**
 * @file KISS.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief KISS encapsultor/unwrapper
 * @version 1.0.1
 * @date 2022-10-23
 *
 * This file is derived from Dire Wolf, an amateur radio packet TNC.
 * Specifically pulled from kiss_frame.c
 *
 *    Copyright (C) 2013, 2014, 2017  John Langner, WB2OSZ
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http:*www.gnu.org/licenses/>.
 *
 */

#ifndef KISS_H
#define KISS_H

#include "constants.h"

#include <LibPrintf.h>

//#include <Arduino.h>

#define DEBUG

int kiss_encapsulate(byte *in, int ilen, byte *out);

int kiss_unwrap(byte *in, int ilen, byte *out);

#endif /* KISS_H */
