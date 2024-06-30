/**
 * @file packetfinder.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief aligns buffer to first 0xC0 and returns length of KISS packet including delimiters; buffer is uint8_t of arbitrary size
 * @version 1.0.1
 * @date 2022-10-23
 *
 * This is a template function that's setup to allow different buffer sizes
 * processbuff returns the length of the KISS datapacket.  Remember that when using HDLC the packet includes CRC bytes tacked on by the radio, but not included in the MTU size used by TNCattach
 */

#ifndef PACKETFINDER_H
#define PACKETFINDER_H

#include <CircularBuffer.h>
#include <LibPrintf.h>
#include <Arduino.h>

#include "constants.h"

template<size_t S>
int processbuff(CircularBuffer<unsigned char, S>& mybuffer)
{
  //there's data in the buffer, but is it a packet?
  //look for 0xC0...we're using direct addressing of the circular buffer because we want to do non-destructive reads
  int bytecount = 0;
  //uint16_t startbyte = 0;

  //first make sure the buffer is not empty! if it's zero, then do nothing.
  if (mybuffer.size() != 0)
  {
    //find the first 0xC0, can also get stuck if there's content in the buffer but no 0xC0 at all, so need to account for that
    while (mybuffer.first() != uint8_t(0xC0) && mybuffer.size() != 0)
    {
      mybuffer.shift(); //will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
    }
    //now jump over any repeats (I kept these separate for clarity)
    //when the shift happens, the value past the end of the buffer size stays the same, so you need something to shift that's not C0, if the buffer size is 1 and it's just C0,
    //when you shift it still says mybuffer[1] is C0, because you're reading past the end of the buffer.
    //When you have a buffer the size of 2 and you shift, you'll end up with a buffer with size 1, so you're still reading past the end of the buffer
    //A size greater than 2 guarantees that after a shift you still are reading a valid location.  (you're looking trying to align to a buffer that has 0xC0, 0xCMD to begin)
    //note that what we're accounting for is 2 or more 0xC0 back to back.  I guess alternatively we could avoid processing until there are at least 3 bytes in the buffer (min cmd size)?
    if (mybuffer.size() > 2)
    {
      while (mybuffer.size() > 2 && mybuffer.first() == uint8_t(0xC0) && mybuffer[1] == uint8_t(0xC0)) //this is in case there are multiple 0xC0's
      //so what's happening if there are more than 2 C0s, is that they all get shifted out and that's because we're reading past the end of the buffer again!
      {
        //debug_printf("before shift mybuffer[1]: %x \r\n", mybuffer[1]);
        //debug_printf("before shift mybuffer.first(): %x \r\n", mybuffer.first());
        //debug_printf("buffer size: %x \r\n", mybuffer.size()); 
        mybuffer.shift(); //will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
        //debug_printf("after shift mybuffer[1]: %x \r\n", mybuffer[1]);
        //debug_printf("after shift mybuffer.first(): %x \r\n", mybuffer.first());
        //debug_printf("buffer size: %x \r\n", mybuffer.size());
      }
    }

    //now find the end if there is one.
    bytecount = 1;

    for (int i = 1; i < mybuffer.size(); i++)
    {
      if (mybuffer[i] == uint8_t(0xC0) && mybuffer[i+1] == uint8_t(0xC0))
      {
        bytecount = i; //this is the size of the packet
        break;
      }
      else if (mybuffer[i] == uint8_t(0xC0) && mybuffer[i+1] != uint8_t(0xC0))
      {
        //the data is bad and we need to purge it.  We know it's bad because there aren't two C0s in succession
        for (int j=0; j < i + 1; j++){
          mybuffer.shift();
        }
        bytecount = 1;
      }
    }

    if (bytecount == 1)
    {
      //a complete packet is not in the buffer
      //debug_printf("No complete packet found \r\n");
      return 0;
    }
    else
    {
      //debug_printf("The packet length is: %u \r\n", bytecount+1);
      //returns length of packet including delimiters
      return (bytecount+1);
    }
  }
  else return 0;
}

#endif /* PACKETFINDER_H */
