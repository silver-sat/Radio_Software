#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\packet.cpp"
/**
* @file packet.cpp
* @author Tom Conrad (tom@silversat.org)
* @brief Library for silversat packet handling and parameter extraction
* @version 1.0.1
* @date 2024-9-17

packet.h - Library for silversat packet handling and parameter extraction
Created by Tom Conrad, September 17, 2024.
Released into the public domain.

*/

#include "packet.h"

Packet::Packet()
{

}

int Packet::extractParams()
{
  //you have a command body string that's delimited by spaces and you want to pull out the parameters
  numparams = 0;
  int body_length = packetlength - 2; //account for existing null terminator
  size_t start_position = 0, end_position;
  String token;
  String packetstring = (char *)packetbody;
  while ((end_position = packetstring.indexOf(" ", start_position)) != -1)
  {
    token = packetstring.substring(start_position, end_position);
    start_position = end_position + 1;
    parameters[numparams] = token;
    numparams++;
  }
  if (packetstring.length() > 0) 
  {
    parameters[numparams] = packetstring.substring(start_position);
    numparams++;
  }

  Log.trace(F("packet body: %s\r\n"), packetstring.c_str());
  Log.trace(F("command code: %X\r\n"), commandcode);
  Log.trace(F("numparams: %d\r\n"), numparams);
  Log.trace(F("parameters: "));

  for(int i=0; i<numparams; i++) Log.trace("%d \r\n", parameters[i].c_str());
  Log.trace("\r\n");
  return numparams; 
}


bool Packet::processcmdbuff(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, CircularBuffer<byte, DATABUFFSIZE> &databuffer)
{
    // first remove the seal... 0xC0
    cmdbuffer.shift();
    // and then grab the command code
    commandcode = cmdbuffer.shift();
    Log.trace(F("command code is: %X\r\n"), commandcode);

    // if (packet.commandcode == 0xAA || packet.commandcode == 0x00) {
    if (commandcode == 0xAA)
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
        Log.trace(F("packetlength = %i\r\n"), packetlength);           // the size of the packet
        Log.trace(F("databuffer length = %i\r\n"), databuffer.size()); // the size that was pushed into the databuffer
        return false;
    }
    else
    {
        // it's possibly a local command
        Log.trace(F("packet length: %i\r\n"), packetlength);
        for (int i = 2; i < (packetlength - 1); i++) // in this case we don't want the last C0
        {
          packetbody[i - 2] = cmdbuffer.shift();
        }
        packetbody[packetlength-3] = 0; // put a null in the next byte...if the command has no body (length =3), then it puts a null in the first byte
        cmdbuffer.shift();                    // remove the last C0 from the buffer

        //Log.notice("command body: %s\r\n", packetbody);
        if (packetlength > 3) extractParams(); //extract the parameters from the command body

        return true;
    }
}