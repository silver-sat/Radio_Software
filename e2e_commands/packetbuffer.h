/**
 * @file packetbuffer.h
 * @author Tom Conrad (tom@silversat.org)
 * @brief packet buffer library for Silversat
 * @version 1.0.1
 * @date 2022-11-08

 */

#define DEBUG
#define MAXRESPONSE 60
#define MAXCOMMANDSIZE 240 //this is the maximum size of a command or command response

#ifndef PACKETBUFFER_H
#define PACKETBUFFER_H


#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif







#endif