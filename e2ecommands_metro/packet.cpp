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
  //you have a command body string that's delimited by spaces and you want to pull out the parameters.  This function populates the vector 'parameters' with the substrings from the packet body.
  numparams = 0;
  int body_length = packetlength - 2; //account for existing null terminator
  size_t start_position = 0, end_position;
  std::string token;
  while ((end_position = packetbody.find(" ", start_position)) != std::string::npos){
    token = packetbody.substr(start_position, end_position-start_position);
    start_position = end_position + 1;
    parameters.push_back(token);
    numparams++;
  }
  parameters.push_back(packetbody.substr(start_position));
  numparams++;
  return numparams; 
}