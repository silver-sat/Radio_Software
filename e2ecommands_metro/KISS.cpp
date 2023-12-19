/**
 * @file KISS.cpp
 * 
 * @author Tom Conrad (tom@silversat.org)
 * @brief KISS encapsultor/unwrapper
 * @version 1.0.1
 * @date 2022-10-23
 * 
 * This file is derived from Dire Wolf, an amateur radio packet TNC.
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
 * @param takes a pointer to the data (*in), the length (ilen), and a pointer to the processed output (*out)
 * @return returns the length of the processed packet
 */


#include "KISS.h"

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

//takes a pointer to the data (*in), the length (ilen), and a pointer to the processed output (*out)
//returns length of encoded packet
int kiss_encapsulate (byte *in, int ilen, byte *out)
{
    int olen;
    int j;

    olen = 0;
    out[olen++] = constants::FEND;
    for (j=0; j<ilen; j++) {

        if (in[j] == constants::FEND) {
            out[olen++] = constants::FESC;
            out[olen++] = constants::TFEND;
        }
        else if (in[j] == constants::FESC) {
            out[olen++] = constants::FESC;
            out[olen++] = constants::TFESC;
        }
        else {
            out[olen++] = in[j];
        }
    }
    out[olen++] = constants::FEND;
    
    return (olen);

}  /* end kiss_encapsulate */  


//takes a pointer to the data (*in), the length (ilen), and a pointer to the processed output (*out)
//returns length of unencoded packet
int kiss_unwrap (byte *in, int ilen, byte *out)
{
    int olen;
    int j;
    int escaped_mode;

    olen = 0;
    escaped_mode = 0;

    if (ilen < 2) {
        /* Need at least the "type indicator" byte and constants::FEND. */
        /* Probably more. */
        debug_printf("KISS message less than minimum length.\n");
        return (0);
    }

    if (in[ilen-1] == constants::FEND) {
        ilen--;	/* Don't try to process below. */
    }
    else {
        debug_printf("KISS frame should end with constants::FEND.\n");
    }

    if (in[0] == constants::FEND) {
        j = 1;	/* skip over optional leading constants::FEND. */
    }
    else {
        j = 0;
    }

    for ( ; j<ilen; j++) {

        if (in[j] == constants::FEND) {
            debug_printf("KISS frame should not have constants::FEND in the middle.\n");
        }

        if (escaped_mode) {

            if (in[j] == constants::TFESC) {
                out[olen++] = constants::FESC;
            }
            else if (in[j] == constants::TFEND) {
                out[olen++] = constants::FEND;
            }
            else {
                debug_printf("KISS protocol error.  Found 0x%02x after constants::FESC.\n", in[j]);
            }
            escaped_mode = 0;
        }
        else if (in[j] == constants::FESC) {
            escaped_mode = 1;
        }
        else {
            out[olen++] = in[j];
        }
    }
        
    return (olen);
        
}
