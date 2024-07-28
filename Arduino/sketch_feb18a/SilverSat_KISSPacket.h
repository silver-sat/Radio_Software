// SilverSat KISSPacket Class
// Contains KISS data and commands, and is modular
// Created 2023-12-18

#include <CircularBuffer.h>

// Direct to Avionics command
#define CMD_AVIONICS = 0xAA

// Constants
const unsigned int BUFFERSIZE{1024}; // bytes
// const unsigned int RADIO_PACKETSIZE{256};
// const unsigned int RADIO_BUFFERSIZE{BUFFERSIZE}; // Could be the AX5043 FIFO size
const char MINPAKCETSIZE{2};

// Classes
// This hold the data and command byte of an unencoded KISS packet
class KISSPacket
{
}