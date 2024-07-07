/*
 PacketBuffer.h - Library for parsing buffers
  Created by Tom Conrad, July 6, 2024.
  Released into the public domain.

  
*/

#ifndef PACKETBUFFER_H
#define PACKETBUFFER_H

#ifndef CMDBUFFSIZE
#define CMDBUFFSIZE 512 // 4 packets at max packet size...but probably a lot more because commands are short
#endif

#ifndef DATABUFFSIZE
#define DATABUFFSIZE 8192 // 32 packets at max packet size.  Need to watch for an overflow on this one!!!
#endif

#include "Arduino.h"
#include "constants.h"
#include <LibPrintf.h>
#include <CircularBuffer.h>
#include <LibPrintf.h>

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(...)
#endif

//this handles the local packets in the serial buffers
typedef struct local_packet
{
    uint16_t length;
    byte command;
    unsigned char body[0x200];
    int CRC;
};

template <size_t S>
class PacketBuffer : public CircularBuffer
{
    public:
        PacketBuffer(size_t S);

        int find_packet();  //returns length of complete packet, 0 if no complete packet found

        local_packet get_packet(int packetlength);

        int kiss_encapsulate(byte *in, int ilen, byte *out);
        int kiss_unwrap(byte *in, int ilen, byte *out);

    private:
        int _packetlength;    
        
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