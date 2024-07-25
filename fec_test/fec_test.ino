/**
 * @file fec_test.ino
 * @author Tom Conrad (tom@silversat.org)
 * @brief test of the libre space FEC library for the AX5043
 * @version 1.0
 * @date 2024-07-25
 *
 * I want to see if we can adapt the librespace FEC library to use with the 
 * Silversat Radio, especially the existing code.
 * 
 * We are currently using variable sized packets with a maximum size of 236 bytes (max MTU size), 
 * but let's start with 204 bytes (MTU size plus 4 TNCattach overhead).
 *
 */

#include "fec.h"

const int packetsize = 204;
unsigned char testpacket[packetsize];


void setup()
{   
    Serial.begin(57600);

    //create a test packet
    for (int i=0; i < packetsize; i++)
    {
        testpacket[i]=random(0xC0);
    }

    Serial.write(testpacket, packetsize);

}

void loop()
{


}