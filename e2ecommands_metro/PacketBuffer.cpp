/*
  PacketBuffer.cpp - Library for parsing buffers
  Created by Tom Conrad, July 6, 2024.
  Released into the public domain.

*/

#include "Arduino.h"
#include "PacketBuffer.h"

template <size_t S>
PacketBuffer::PacketBuffer(size_t S) : CircularBuffer(<byte, S>)
{

}

//template <size_t S>
int PacketBuffer::find_packet()
{
    // there's data in the buffer, but is it a packet?
    // look for 0xC0...we're using direct addressing of the circular buffer because we want to do non-destructive reads
    int bytecount = 0;
    // uint16_t startbyte = 0;

    // first make sure the buffer is not empty! if it's zero, then do nothing.
    if (mybuffer.size() != 0)
    {
        // find the first 0xC0, can also get stuck if there's content in the buffer but no 0xC0 at all, so need to account for that
        while (mybuffer.first() != uint8_t(0xC0) && mybuffer.size() != 0)
        {
            mybuffer.shift(); // will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
        }
        // now jump over any repeats (I kept these separate for clarity)
        // when the shift happens, the value past the end of the buffer size stays the same, so you need something to shift that's not C0, if the buffer size is 1 and it's just C0,
        // when you shift it still says mybuffer[1] is C0, because you're reading past the end of the buffer.
        // When you have a buffer the size of 2 and you shift, you'll end up with a buffer with size 1, so you're still reading past the end of the buffer
        // A size greater than 2 guarantees that after a shift you still are reading a valid location.  (you're looking trying to align to a buffer that has 0xC0, 0xCMD to begin)
        // note that what we're accounting for is 2 or more 0xC0 back to back.  I guess alternatively we could avoid processing until there are at least 3 bytes in the buffer (min cmd size)?
        if (mybuffer.size() > 2)
        {
            while (mybuffer.size() > 2 && mybuffer.first() == uint8_t(0xC0) && mybuffer[1] == uint8_t(0xC0)) // this is in case there are multiple 0xC0's
            // so what's happening if there are more than 2 C0s, is that they all get shifted out and that's because we're reading past the end of the buffer again!
            {
                // debug_printf("before shift mybuffer[1]: %x \r\n", mybuffer[1]);
                // debug_printf("before shift mybuffer.first(): %x \r\n", mybuffer.first());
                // debug_printf("buffer size: %x \r\n", mybuffer.size());
                mybuffer.shift(); // will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
                // debug_printf("after shift mybuffer[1]: %x \r\n", mybuffer[1]);
                // debug_printf("after shift mybuffer.first(): %x \r\n", mybuffer.first());
                // debug_printf("buffer size: %x \r\n", mybuffer.size());
            }
        }

        // now find the end if there is one.
        bytecount = 1;

        for (int i = 1; i < mybuffer.size(); i++)
        {
            if (mybuffer[i] == uint8_t(0xC0) && mybuffer[i + 1] == uint8_t(0xC0))
            {
                bytecount = i; // this is the size of the packet
                break;
            }
            else if (mybuffer[i] == uint8_t(0xC0) && mybuffer[i + 1] != uint8_t(0xC0))
            {
                // the data is bad and we need to purge it.  We know it's bad because there aren't two C0s in succession
                for (int j = 0; j < i + 1; j++)
                {
                    mybuffer.shift();
                }
                bytecount = 1;
            }
        }

        if (bytecount == 1)
        {
            // a complete packet is not in the buffer
            // debug_printf("No complete packet found \r\n");
            return 0;
        }
        else
        {
            // debug_printf("The packet length is: %u \r\n", bytecount+1);
            // returns length of packet
            return (bytecount + 1);
        }
    }
    else
        return 0;
}

local_packet PacketBuffer::get_packet()
{
    // there's data in the buffer, but is it a packet?
    // look for 0xC0...we're using direct addressing of the circular buffer because we want to do non-destructive reads
    int bytecount = 0;
    // uint16_t startbyte = 0;

    // first make sure the buffer is not empty! if it's zero, then do nothing.
    if (mybuffer.size() != 0)
    {
        // find the first 0xC0, can also get stuck if there's content in the buffer but no 0xC0 at all, so need to account for that
        while (mybuffer.first() != uint8_t(0xC0) && mybuffer.size() != 0)
        {
            mybuffer.shift(); // will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
        }
        // now jump over any repeats (I kept these separate for clarity)
        // when the shift happens, the value past the end of the buffer size stays the same, so you need something to shift that's not C0, if the buffer size is 1 and it's just C0,
        // when you shift it still says mybuffer[1] is C0, because you're reading past the end of the buffer.
        // When you have a buffer the size of 2 and you shift, you'll end up with a buffer with size 1, so you're still reading past the end of the buffer
        // A size greater than 2 guarantees that after a shift you still are reading a valid location.  (you're looking trying to align to a buffer that has 0xC0, 0xCMD to begin)
        // note that what we're accounting for is 2 or more 0xC0 back to back.  I guess alternatively we could avoid processing until there are at least 3 bytes in the buffer (min cmd size)?
        if (mybuffer.size() > 2)
        {
            while (mybuffer.size() > 2 && mybuffer.first() == uint8_t(0xC0) && mybuffer[1] == uint8_t(0xC0)) // this is in case there are multiple 0xC0's
            // so what's happening if there are more than 2 C0s, is that they all get shifted out and that's because we're reading past the end of the buffer again!
            {
                // debug_printf("before shift mybuffer[1]: %x \r\n", mybuffer[1]);
                // debug_printf("before shift mybuffer.first(): %x \r\n", mybuffer.first());
                // debug_printf("buffer size: %x \r\n", mybuffer.size());
                mybuffer.shift(); // will remove what's at the head until it gets to an 0xC0, so the head should always contain an 0xC0
                // debug_printf("after shift mybuffer[1]: %x \r\n", mybuffer[1]);
                // debug_printf("after shift mybuffer.first(): %x \r\n", mybuffer.first());
                // debug_printf("buffer size: %x \r\n", mybuffer.size());
            }
        }

        // now find the end if there is one.
        bytecount = 1;

        for (int i = 1; i < mybuffer.size(); i++)
        {
            if (mybuffer[i] == uint8_t(0xC0) && mybuffer[i + 1] == uint8_t(0xC0))
            {
                bytecount = i; // this is the size of the packet
                break;
            }
            else if (mybuffer[i] == uint8_t(0xC0) && mybuffer[i + 1] != uint8_t(0xC0))
            {
                // the data is bad and we need to purge it.  We know it's bad because there aren't two C0s in succession
                for (int j = 0; j < i + 1; j++)
                {
                    mybuffer.shift();
                }
                bytecount = 1;
            }
        }

        if (bytecount == 1)
        {
            // a complete packet is not in the buffer
            // debug_printf("No complete packet found \r\n");
            return 0;
        }
        else
        {
            // debug_printf("The packet length is: %u \r\n", bytecount+1);
            // returns length of packet
            return (bytecount + 1);
        }
    }
    else
        return 0;
}

int PacketBuffer::kiss_encapsulate(byte *in, int ilen, byte *out)
{
    int olen;
    int j;

    olen = 0;
    out[olen++] = constants::FEND;
    for (j = 0; j < ilen; j++)
    {

        if (in[j] == constants::FEND)
        {
            out[olen++] = constants::FESC;
            out[olen++] = constants::TFEND;
        }
        else if (in[j] == constants::FESC)
        {
            out[olen++] = constants::FESC;
            out[olen++] = constants::TFESC;
        }
        else
        {
            out[olen++] = in[j];
        }
    }
    out[olen++] = constants::FEND;

    return (olen);
}

int PacketBuffer::kiss_unwrap(byte *in, int ilen, byte *out)
{
    int olen;
    int j;
    int escaped_mode;

    olen = 0;
    escaped_mode = 0;

    if (ilen < 2)
    {
        /* Need at least the "type indicator" byte and constants::FEND. */
        /* Probably more. */
        debug_printf("KISS message less than minimum length.\n");
        return (0);
    }

    if (in[ilen - 1] == constants::FEND)
    {
        ilen--; /* Don't try to process below. */
    }
    else
    {
        debug_printf("KISS frame should end with constants::FEND.\n");
    }

    if (in[0] == constants::FEND)
    {
        j = 1; /* skip over optional leading constants::FEND. */
    }
    else
    {
        j = 0;
    }

    for (; j < ilen; j++)
    {

        if (in[j] == constants::FEND)
        {
            debug_printf("KISS frame should not have constants::FEND in the middle.\n");
        }

        if (escaped_mode)
        {

            if (in[j] == constants::TFESC)
            {
                out[olen++] = constants::FESC;
            }
            else if (in[j] == constants::TFEND)
            {
                out[olen++] = constants::FEND;
            }
            else
            {
                debug_printf("KISS protocol error.  Found 0x%02x after constants::FESC.\n", in[j]);
            }
            escaped_mode = 0;
        }
        else if (in[j] == constants::FESC)
        {
            escaped_mode = 1;
        }
        else
        {
            out[olen++] = in[j];
        }
    }

    return (olen);
}