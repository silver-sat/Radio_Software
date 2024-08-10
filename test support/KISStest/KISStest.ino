/*  @file KISStest.c
    @author Tom Conrad (tom@silversat.org)
    @brief KISS encapsultor/unwrapper test
    @version 1.0.1
    @date 2022-10-23

    This file is derived from Dire Wolf, an amateur radio packet TNC.

    Copyright (C) 2013, 2014, 2017  John Langner, WB2OSZ

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http:*www.gnu.org/licenses/>.

*/

#include "KISS.h"
#include <assert.h>
#include <LibPrintf.h>

void setup()
{
  Serial.begin(115200);
  delay(5000);

  printf("starting test \n");

  uint8_t din[512];
  uint8_t kissed[520];
  uint8_t dout[520];
  int klen;
  int dlen;
  int k;

  for (k = 0; k < 512; k++) {
    if (k < 256) {
      din[k] = k;
    }
    else {
      din[k] = 511 - k;
    }
  }

  klen = kiss_encapsulate(din, 512, kissed);
  assert (klen == 512 + 6);

  dlen = kiss_unwrap(kissed, klen, dout);
  assert (dlen == 512);
  assert (memcmp(din, dout, 512) == 0);

  dlen = kiss_unwrap(kissed + 1, klen - 1, dout);
  assert (dlen == 512);
  assert (memcmp(din, dout, 512) == 0);

  printf("Quick KISS test passed OK.\n");
  //exit (EXIT_SUCCESS);

}

void loop()
{

  /* add main program code here, this code starts again each time it ends */

}
