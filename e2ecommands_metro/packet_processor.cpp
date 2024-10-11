/**
 * @file packet_processor.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief packet processor for Silversat
 * @version 1.0.1
 * @date 2024-10-07
 *
 * This code doesn't actually do anything yet.  It's more of a concept toward storing unencoded packets in the 
 * buffers as packet objects rather than kiss.  
 *
 */

#include "packet_processor.h"


bool processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, CommandPacket &packet)
{
    // first remove the seal... 0xC0
    cmdbuffer.shift();
    // and then grab the command code
    packet.commandcode = cmdbuffer.shift();
    Log.trace("command code is: %X\r\n", packet.commandcode);

    // if (packet.commandcode == 0xAA || packet.commandcode == 0x00) {
    if (packet.commandcode == 0xAA)
    {
        // nothing to see here, it's not for me...forward to the other end, so copy this over to the tx buffer
        databuffer.push(constants::FEND);
        // so for commands or responses bound for the other side, I'm adding a new command code back on to indicate where it's going.
        databuffer.push(0xAA);
        // you're starting at the second byte of the total packet
        // noInterrupts();  //turn off interrupts until this is done.  This is to avoid writing to the buffer until all the packet is shifted out.
        for (int i = 2; i < packetlength; i++)
        {
            // shift it out of cmdbuffer and push it into databuffer, don't need to push a final 0xC0 because it's still part of the packet
            databuffer.push(cmdbuffer.shift());
        }
        // interrupts();
        Log.trace("packetlength = %i\r\n", packetlength);           // the size of the packet
        Log.trace("databuffer length = %i\r\n", databuffer.size()); // the size that was pushed into the databuffer
        return false;
    }
    else
    {
        // it's possibly a local command
        Log.trace("packet length: %i\r\n", packetlength);
        for (int i = 2; i < (packetlength - 1); i++) // in this case we don't want the last C0
        {
          packet.packetbody[i - 2] = cmdbuffer.shift();
        }
        packet.packetbody[packetlength-3] = 0; // put a null in the next byte...if the command has no body (length =3), then it puts a null in the first byte
        cmdbuffer.shift();                    // remove the last C0 from the buffer

        Log.notice("command body: %s\r\n", packet.packetbody);

        packet.packetlength = packetlength; // the total packet including framing
        packet.extractParams(); //extract the parameters from the command body
        return true;
    }
}


bool processdatabuff(CircularBuffer<byte, DATABUFFSIZE> &databuffer, int packetlength, DataPacket &datapacket)
{
   return false;
}






