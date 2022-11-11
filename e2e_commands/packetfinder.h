/**
 * @file packetfinder.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief aligns buffer to first 0xC0 and returns length of KISS packet including delimiters; buffer is uint8_t of arbitrary size
 * @version 1.0.1
 * @date 2022-10-23
 *
 *
 */

#ifndef PACKETFINDER_H
#define PACKETFINDER_H


#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 //32 packets at max packet size
#endif

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512  //4 packets at max packet size...but probably a lot more because commands are short
#endif

#define DEBUG

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#include <CircularBuffer.h>
#include <LibPrintf.h>


template<size_t S>
int processbuff(CircularBuffer<unsigned char, S>& mybuffer)
{
  //there's data in the buffer, but is it a packet?
  //look for 0xC0...we're using direct addressing of the circular buffer because we want to do non-destructive reads
  int bytecount = 0;
  uint16_t startbyte = 0;

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
      while (mybuffer.first() == uint8_t(0xC0) && mybuffer[1] == uint8_t(0xC0)) //this is in case there are multiple 0xC0's
      {
        mybuffer.shift(); //will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
        //debug_printf("mybuffer[1]: %x \n", mybuffer[1]);
      }
    }

    //now find the end if there is one.
    bytecount = 1;

    for (int i = 1; i < mybuffer.size(); i++)
    {
      if (mybuffer[i] == uint8_t(0xC0))
      {
        bytecount = i; //this is the size of the packet
        break;
      }
    }

    if (bytecount == 1)
    {
      //a complete packet is not in the buffer
      debug_printf("No complete packet found \n");
      return 0;
    }
    else
    {
      debug_printf("The packet length is: %x \n", bytecount+1);
      //returns length of packet
      return (bytecount+1);
    }
  }
  else return 0;
}

#endif /* PACKETFINDER_H */