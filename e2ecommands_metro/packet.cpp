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

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

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
  String packetstring = String(packetbody);
  while ((end_position = packetstring.indexOf(" ", start_position)) != -1){
    token = packetstring.substring(start_position, end_position);
    start_position = end_position + 1;
    parameters[numparams] = token;
    numparams++;
  }
  if (packetstring.length() > 0) {
    parameters[numparams] = packetstring.substring(start_position);
    numparams++;
  }

  debug_printf("packet body: %s \r\n", packetstring.c_str());
  debug_printf("command code: %x \r\n", commandcode);
  debug_printf("numparams: %d \r\n", numparams);
  debug_printf("parameters: ");

  for(int i=0; i<numparams; i++) debug_printf("%u ", parameters[i].c_str());
  debug_printf("\r\n");
  return numparams; 
}