/*
 PacketProcessor.h - Library for using the eFuse, specifically a TPS25940LQRVCRQ1
  Created by Tom Conrad, July 6, 2024.
  Released into the public domain.

  
*/

#ifndef PACKETPROCESSOR_H
#define PACKETPROCESSOR_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512 // 4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 // 32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include "Arduino.h"
#include <LibPrintf.h>
#include <CircularBuffer.h>

class PacketProcessor
{
    public:
        PacketProcessor(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer);

        template <size_t S>
        int find_packet(CircularBuffer<unsigned char, S> &mybuffer);  //returns length of complete packet, 0 if no complete packet found

        void get_command(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, int packetlength);
        void get_body(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, int packetlength);
        void get_crc(CircularBuffer<byte, CMDBUFFSIZE> &cmdbuffer, int packetlength);

        void sendACK(byte code);
        void sendNACK(byte code);

    private:
        int _packetlength;
        byte _command;
        
        
};

#endif

/*
class Command
{
    public:
        Command(uint8_t code);

        void beacon();               //2.3.1.1
        void manualAntennaRelease(); // 2.3.1.2
        void status();               // 2.3.1.3
        void halt();                 // 2.3.1.4
        void modify_freq();          // 2.3.1.5
        void modify_mode();          // 2.3.1.6
        void doppler_frequencies();  // 2.3.1.7
        void transmit_callsign();    // 2.3.1.8

        void transmit_cw();  //2.3.2.1
        void background_rssi(); // 2.3.2.2
        void current_rssi();    // 2.3.2.3
        void sweep_transmitter(); // 2.3.2.4
        void sweep_receiver();    // 2.3.2.5
        void query_radio_register(); // 2.3.2.6
        void adjust_output_power();  // 2.3.2.7

    private:
        void ACK();
        void NACK();
        void Respond();

};

*/