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
#include <LibPrintf.h>
#include <CRC32.h>

const int packetsize = 204;
unsigned char testpacket[packetsize];


void setup()
{   
    Serial.begin(57600);
    randomSeed(analogRead(0));

    while (!Serial);  //wait for serial monitor to open

    //create a test packet
    for (int i=0; i < packetsize; i++)
    {
        testpacket[i]=random(0x21, 0x73);
    }
    printf("Test Packet: \r\n");
    Serial.write(testpacket, packetsize);
    printf("\r\n");

    //calculate its CRC value
    uint32_t initialCRC = CRC32::calculate(testpacket,packetsize);
    printf("CRC32: %u \r\n", initialCRC);
    printf("\r\n");

    unsigned char paritydata[32];

    //encode the packet. it populates the paritydata array.  While you're at it, calculate the time to do the encoding.
    int startcalc = micros();
    rs_encode(paritydata, testpacket, packetsize);
    int encodetime = micros()-startcalc;
    printf("time to encode: %i \r\n", encodetime);

    printf("Parity data: \r\n");
    for (int i=0; i<32; i++)
    {
      printf("%u ,",paritydata[i]);
    }
    printf("\r\n");

    //put the test packet bytes into the new packet array
    unsigned char newpacket[packetsize];
    for (int i=0; i<packetsize; i++){
      newpacket[i] = testpacket[i];
    }

    //now add the parity bytes to the end of the packet
    for (int i=packetsize; i<packetsize+32; i++){
      newpacket[i] = paritydata[i-packetsize];
    }

    //now mess things up
    int badbytes = 16;  //this is the number of errors we're creating
    unsigned char badbytelocations[badbytes];
    for (int i=0; i<badbytes; i++)
    {
      badbytelocations[i] = random(packetsize+32);  //yes, it's possible to have dups, and it's also possible to cause an error in the parity bits
      newpacket[badbytelocations[i]] = newpacket[badbytelocations[i]] | 0x80;  
      //replace the first bit in the previous defined location with the same byte OR'd with 0x80.  That takes the byte out of printable range
    }
    
    //print out the new packet
    Serial.write(newpacket, packetsize+32); //this should be the packet including the bad bytes
    printf("\r\n");
    
    //calculate the CRC of the packet with the bad bits (just leave out the parity bits)
    startcalc = micros();
    uint32_t midpointCRC = CRC32::calculate(newpacket,packetsize);
    printf("CRC32: %u \r\n", midpointCRC);
    int calctime=micros()-startcalc;
    printf("time to calculate: %i \r\n", calctime);
    printf("\r\n");

    //report where the bad bytes are supposed to be
    printf("The bad bytes are in positions: ");
    for (int i=0; i<badbytes; i++)
    {
      printf("%d, ", badbytelocations[i]);
    }
    printf("\r\n");

    //no decode the packet
    startcalc = micros();
    int corrected_bytes = rs_decode(newpacket, packetsize+32);
    int decodetime = micros()-startcalc;
    printf("time to decode: %i \r\n", decodetime);

    //the decoder returns the number of corrected bytes  (remember multiple corrupted bits in a byte are treated as a single error...see wikipedia)
    printf("Number of corrected bytes: %i \r\n", corrected_bytes);
    Serial.write(newpacket, packetsize);  //this should be the corrected packet
    printf("\r\n");
    uint32_t finalCRC = CRC32::calculate(newpacket,packetsize);
    printf("CRC32: %u \r\n", finalCRC);

    //wrap it all up!
    if (finalCRC - initialCRC == 0) printf("YAY!!!!!\r\n");
    else printf("NOOOOOOOOOO!\r\n");

}

void loop()
{

}