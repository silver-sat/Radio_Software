/**
 * @file packetfinder.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief aligns buffer to first 0xC0 and returns length of KISS packet including delimiters; buffer is uint8_t of arbitrary size
 * @version 1.0.1
 * @date 2024-7-23
 *
 * This is a template function that's setup to allow different buffer sizes
 * processbuff returns the length of the KISS datapacket.  
 * Remember that when using HDLC, the packet includes CRC bytes tacked on by the radio, 
 * but these are not included in the MTU size used by TNCattach (if you're doing accounting of packet sizes)
 */

#ifndef PACKETFINDER_H
#define PACKETFINDER_H

//#define DEBUG

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#include <CircularBuffer.h>
#include <LibPrintf.h>
#include <Arduino.h>

#include "constants.h"

template<size_t S>
int processbuff(CircularBuffer<unsigned char, S>& mybuffer)
{
    //there's data in the buffer, but is it a packet?
    //look for 0xC0...we're using direct addressing of the circular buffer because we want to do non-destructive reads
    int bytecount = 1;
    //uint16_t startbyte = 0;

    //find the first 0xC0, can also get stuck if there's content in the buffer but no 0xC0 at all, so need to account for that
    while (mybuffer.first() != uint8_t(0xC0) && mybuffer.size() != 0)
    {
        mybuffer.shift(); // will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
    }

    // now jump over any repeated C0s (I kept these separate for clarity)
    // when the shift happens, the value past the end of the buffer size stays the same, so you need something to shift that's not C0, if the buffer size is 1 and it's just C0,
    // when you shift it still says mybuffer[1] is C0, because you're reading past the end of the buffer.
    // When you have a buffer the size of 2 and you shift, you'll end up with a buffer with size 1, so you're still reading past the end of the buffer
    // A size greater than 2 guarantees that after a shift you still are reading a valid location.  (you're looking trying to align to a buffer that has 0xC0, 0xCMD to begin)
    // note that what we're accounting for is 2 or more 0xC0 back to back.  I guess alternatively we could avoid processing until there are at least 3 bytes in the buffer (min cmd size)?

    while (mybuffer.size() > 2 && mybuffer.first() == uint8_t(0xC0) && mybuffer[1] == uint8_t(0xC0)) // this is in case there are multiple 0xC0's
    // so what's happening if there are more than 2 C0s, is that they all get shifted out and that's because we're reading past the end of the buffer again!
    {
        mybuffer.shift(); // will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
    }

    //now make sure the buffer contains something larger than the minimal size command. otherwise do nothing.  min size command is 3.  0xC0 0x?? 0xC0.
    //min size data is larger because of the tncattach header.
    if (mybuffer.size() < 3) 
        {
            for (int j=0; j < mybuffer.size(); j++)
            {
                debug_printf("buffer contents [%i]: %x \r\n", j, mybuffer[j]);
            }
            return 0;
        }

    //now find the end if there is one.
    //we should have something like 0xC0 0x?? 0x?? in the buffer at the very least since we're checking for a size greater than 3
    //bytecount = 1;

    for (int i = 2; i < mybuffer.size(); i++)  //start at the third byte
    {
        if (mybuffer[i] == uint8_t(0xC0))
        {
            bytecount = i; //this is the size of the packet less the final C0
            break;
        }
    }
    //if no C0 is found, then we've reached the end of the buffer
    if (bytecount == 1)
    {
        //a complete packet is not in the buffer
        /*
        debug_printf("buffer size: %i \r\n", mybuffer.size());
        for (int j=0; j < mybuffer.size(); j++)
            {
                debug_printf("buffer contents [%i]: %x \r\n", j, mybuffer[j]);
            }
            return 0;
        debug_printf("No complete packet found \r\n");
        */
        return 0;
    }
    else
    {
        //returns length of packet, which is the bytecount plus the extra C0.
        debug_printf("The packet length is: %u \r\n", bytecount+1);
        //trying to catch the bug
        /*
        if (bytecount + 1 < 198)
        {
          for (int j=0; j < mybuffer.size(); j++)
            {
                debug_printf("buffer contents [%i]: %x \r\n", j, mybuffer[j]);
            }
            //while(1);
        }
        */ 
        return (bytecount+1);
    }
}

#endif /* PACKETFINDER_H */
