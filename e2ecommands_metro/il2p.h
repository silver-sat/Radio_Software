/**
* @file il2p.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library for provide il2p support
* @version 1.0.1
* @date 2024-10-8

il2p.h - Library for provide il2p support
Created by Tom Conrad, October 8, 2024.
Released into the public domain.

This library was designed and optimized for Silversat. It assumes a *mostly* fixed il2p header.
That is, the call signs are not dynamic, nor are the SSIDs.
We are only sending UI packets (but might add *connected mode* using another method, so that might change)

I'm also assuming that we're connecting with tncattach and using RAW packets
That means that there are 4 extra tncattach bytes, the command byte, and the length byte added to each
packet that comes in from payload.  

I've precompiled the header for 255 byte packets.  Anything smaller will still use the same encoder/decoder,
but will be padded to get to 255 (for encoding/decoding purposes).  The padding is not transmitted.
Since there are ultimately 32 RS parity bytes, the payload length is limited to 255-32-2 = 221.  The extra
2 are the command byte (I need to send it to support commands), and the length byte, which is also only needed
for the Silversat main mission.

The four extra tncattach bytes are only present on the main mission assuming that payload is writing packets
directly to the port (not IP packets)

Our implementation takes advantage of the packet processing capability in the AX5043.  We use the 0x55 preambles
and have added the Frame sync bytes to the preamble/frame sync code in AX.cpp.  They're used to help change 
RF parameter sets once it has preamble lock.

Our packet will always start with the command byte and the length byte.  The actual IL2P packet will follow.
I'm going to set up for two options.  One where the AX5043 CRC checking is enabled (where the CRC-16 bytes are added)
And one where the CRC checking is disabled.  I believe I can configure it to not add the CRC bytes, so we will be
relying on the IL2P CRC check, which now needs to be implemented as well.  Thanks.  And yes, i mean that sarcastically.

//example scrambler call:  il2p_scramble_block(il2p_header_raw_example, il2p_header_scrambled, 13);
//example descrambler call il2p_descramble_block(il2p_header_scrambled, il2p_header_unscrambled, 13);

*/

#ifndef IL2P_H
#define IL2P_H

#include "Arduino.h"
#include "packet.h"
#include "ArduinoLog.h"

void il2p_scramble_block (unsigned char *in, unsigned char *out, int len);
void il2p_descramble_block (unsigned char *in, unsigned char *out, int len);

#define INIT_TX_LSFR 0x00F

static inline int scramble_bit (int in, int *state)
{
  int out =((*state >> 4) ^ *state) & 1;
  *state = ((((in^*state) & 1) << 9) | (*state ^ ((*state & 1) << 4))) >> 1;
  return (out);
}

#define INIT_RX_LSFR 0x1F0

static inline int descramble_bit (int in, int *state)
{
  int out = (in ^ *state) & 1;
  *state = ( (*state >> 1) | ((in & 1) << 8)) ^ ((in & 1) << 3);
  return (out);
}


# endif