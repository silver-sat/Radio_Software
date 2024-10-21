#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\il2p.cpp"
/**
* @file il2p.cpp
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

I'm also assuming that we're going to use the existing RS(255, 223) encoder/decoder.

I've precompiled the header for 255 byte packets.  Anything smaller will still use the same encoder/decoder,
but will be padded to get to 255 (for encoding/decoding purposes).  The padding is not transmitted.
Since there are ultimately 32 RS parity bytes, the payload length is limited to 255-32-2 = 221.  The extra
2 are the command byte (I need to send it to support commands), and the length byte, which is also only needed
for the Silversat main mission.

The four extra tncattach bytes are only present on the main mission assuming that payload is writing packets
directly to the port (not IP packets)

//example scrambler call:  il2p_scramble_block(il2p_header_raw_example, il2p_header_scrambled, 13);
//example descrambler call il2p_descramble_block(il2p_header_scrambled, il2p_header_unscrambled, 13);

*/

#include "il2p.h"

//this one is useful for SSDV
unsigned char il2p_header_precoded_255[13]{0x6B, 0xE3, 0x41, 0x76, 0xF6, 0xB7, 0xAB, 0xA3, 0x81, 0xB6, 0xF6, 0xF7, 0x10};
//this one is used to build new headers with variable size packets
unsigned char il2p_header_precoded_0[13]{0x6B, 0xE3, 0x41, 0x76, 0x76, 0x37, 0x2B, 0x23, 0x01, 0x36, 0x76, 0x77, 0x10};
//this one is used to test the scrambler.  disable it when not needed.
unsigned char il2p_header_raw_example[13]{0x63, 0xF1, 0x40, 0x40, 0x40, 0x0, 0x6B, 0x2B, 0x54, 0x28, 0x25, 0x2A, 0x0F};
//and this one went wee all the way home!
unsigned char il2p_header_scrambled[13];
unsigned char il2p_header_unscrambled[13];

/*--------------------------------------------------------------------------------
 *
 * Function:  il2p_scramble_block
 *
 * Purpose: Scramble a block before adding RS parity.
 *
 * Inputs:  in    Array of bytes.
 *    len   Number of bytes both in and out.
 *
 * Outputs: out   Array of bytes.
 * 
 * 
 * ib = input byte
 * im = input mask
 * ob = output byte
 * om = output mask
 *
 *--------------------------------------------------------------------------------*/

void il2p_scramble_block (unsigned char *in, unsigned char *out, int len)
{
  int tx_lfsr_state = INIT_TX_LSFR;

  memset (out, 0, len);

  int skipping = 1; // Discard the first 5 out.
  int ob = 0;   // Index to output byte.
  int om = 0x80;    // Output bit mask;
  for (int ib = 0; ib < len; ib++) {
      for (int im = 0x80; im != 0; im >>= 1) {
          int s = scramble_bit((in[ib] & im) != 0, &tx_lfsr_state);
          if (ib == 0 && im == 0x04) skipping = 0;
          if ( ! skipping) {
              if (s) {
                  out[ob] |= om;
              }
              om >>= 1;
              if (om == 0) {
                  om = 0x80;
                  ob++;
              }
          }
      }
  }
  // Flush it.

  // This is a relic from when I thought the state would need to
  // be passed along for the next block.
  // Preserve the LSFR state from before flushing.
  // This might be needed as the initial state for later payload blocks.
  int x = tx_lfsr_state;
  for (int n = 0; n < 5; n++) {
      int s = scramble_bit(0, &x);
      if (s) {
          out[ob] |= om;
      }
      om >>=1;
      if (om == 0) {
          om = 0x80;
          ob++;
      }
  }

}  // end il2p_scramble_block

/*--------------------------------------------------------------------------------
 *
 * Function:  il2p_descramble_block
 *
 * Purpose: Descramble a block after removing RS parity.
 *
 * Inputs:  in    Array of bytes.
 *    len   Number of bytes both in and out.
 *
 * Outputs: out   Array of bytes.
 *
 *--------------------------------------------------------------------------------*/

void il2p_descramble_block (unsigned char *in, unsigned char *out, int len)
{
  int rx_lfsr_state = INIT_RX_LSFR;

  memset (out, 0, len);

  for (int b = 0; b < len; b++) {
      for (int m = 0x80; m != 0; m >>= 1) {
          int d = descramble_bit((in[b] & m) != 0, &rx_lfsr_state);
          if (d) {
              out[b] |= m;
          }
      }
  }
}

// end il2p_scramble.c
