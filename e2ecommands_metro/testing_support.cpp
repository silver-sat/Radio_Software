/**
 * @file testing_support.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief testing support functions for AX5043 radio and Silversat radio board
 * @version 1.0.1
 * @date 2023-2-27

 */

#include "testing_support.h"

void printRegisters(Radio &radio)
{
    Log.verbose(F("Here are the AX5043 register contents \r\n"));

    // this prints all but ADC, low power oscillator and DAC registers

    Log.verbose(F("registers below 0x70, general config \r\n"));
    for (uint16_t reg = 0; reg < 0x70; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("receiver parameters \r\n"));
    // range 0x100 to 0x118
    for (uint16_t reg = 0x100; reg < 0x119; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("receiver parameter set 0 \r\n"));
    // range 0x120 to 0x12F
    for (uint16_t reg = 0x120; reg < 0x130; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("receiver parameter set 1 \r\n"));
    // range 0x130 to 0x13F
    for (uint16_t reg = 0x130; reg < 0x140; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("receiver parameter set 2 \r\n"));
    // range 0x140 to 0x14F
    for (uint16_t reg = 0x140; reg < 0x150; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("receiver parameter set 3 \r\n"));
    // range 0x150 to 0x15F
    for (uint16_t reg = 0x150; reg < 0x160; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("transmitter parameters \r\n"));
    // range 0x160 to 0x171
    for (uint16_t reg = 0x160; reg < 0x172; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("pll and crystal parameters \r\n"));
    // range 0x180 to 0x189
    for (uint16_t reg = 0x180; reg < 0x18A; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("packet format \r\n"));
    // range 0x200 to 0x20B
    for (uint16_t reg = 0x200; reg < 0x20C; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("pattern match \r\n"));
    // range 0x210 to 0x21E
    for (uint16_t reg = 0x210; reg < 0x21F; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("packet controller \r\n"));
    // range 0x220 to 0x233
    for (uint16_t reg = 0x220; reg < 0x234; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }

    Log.verbose(F("special functions \r\n"));
    // 0x248, 24A, 250, 255
    Log.verbose(F("register= %2x | value= %2x \r\n"), 0x248, 
           radio.getRegValue(0x248));
    Log.verbose(F("register= %2x | value= %2x \r\n"), 0x24A,
           radio.getRegValue(0x24A));
    Log.verbose(F("register= %2x | value= %2x \r\n"), 0x250,
           radio.getRegValue(0x250));
    Log.verbose(F("register= %2x | value= %2x \r\n"), 0x255,
           radio.getRegValue(0x255));

    Log.verbose(F("ADC & low power oscillator \r\n"));
    // range 0x300 to 0x332
    for (uint16_t reg = 0x300; reg < 0x332; reg++)
    {
        Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
               radio.getRegValue(reg));
    }
    // On semi recommends not touching these..even reading from them.
    /*
    Log.verbose(F("peformance tuning registers \r\n"));
    // range 0xF00 to 0xFFF
    for (uint16_t reg = 0xF00; reg <= 0xFFF; reg++)
    {
           Log.verbose(F("register= %2x | value= %2x \r\n"), reg,
           radio.getRegValue(reg));
    }
    */
}

void efuseTesting(Efuse &efuse, ExternalWatchdog &watchdog)
{
    while (true)
    {
        int start_time = millis();
        Log.verbose(F("current: %5f \r\n"), efuse.measure_current());
        Log.verbose(F("execution time: %lx \r\n"), millis() - start_time);
        watchdog.trigger();
        efuse.overcurrent(true);
        // efuse.overcurrent(false);
        delay(1000); // run once per second
    }
}

void il2p_testing()
{
       
       unsigned char il2p_header_raw_example[13]{0x63, 0xF1, 0x40, 0x40, 0x40, 0x0, 0x6B, 0x2B, 0x54, 0x28, 0x25, 0x2A, 0x0F};
       unsigned char il2p_header_precoded[13]   {0x6B, 0xE3, 0x41, 0x76, 0xF6, 0xB7, 0x2B, 0x23, 0x81, 0x36, 0x76, 0x77, 0x10};
       unsigned char il2p_header_scrambled[13];
       int header_size = sizeof(il2p_header_raw_example);
       il2p_scramble_block (il2p_header_raw_example, il2p_header_scrambled, sizeof(il2p_header_raw_example));
       Serial.println(F("Scrambled Header: \r\n"));

       for (int i=0; i < header_size; i++)
       {
       
       Log.trace(F("header: %X \r\n"), il2p_header_scrambled[i]);
       }
       //Log.verbose(F("\r\n"));

       il2p_init();  //must be called first to set up RS tables.  TODO: There is no need to build all the tables, so look at pre-compiling these.
       int parity_size = 2;
       unsigned char parity[parity_size];
       //void il2p_encode_rs (unsigned char *tx_data, int data_size, int num_parity, unsigned char *parity_out);
       il2p_encode_rs(il2p_header_precoded, header_size, parity_size, parity);

       Log.verbose(F("Parity Bytes: \r\n"));
       for (int i=0; i< parity_size; i++) Log.trace(F("P%i: %X\r\n"), i, parity[i]);

       //int il2p_decode_rs (unsigned char *rec_block, int data_size, int num_parity, unsigned char *out)
       unsigned char decode_block[15];
       for (int i=0; i<header_size; i++) decode_block[i]=il2p_header_scrambled[i];
       for (int i=header_size; i<(header_size+2); i++) decode_block[i] = parity[i-header_size];
       unsigned char decoded_data[15];
       int decode_success = il2p_decode_rs(decode_block, 13, 2, decoded_data);

       Log.trace(F("decode_success: %i\r\n"), decode_success);
       for (int i=0; i<(header_size+2); i++) Log.verbose("%X, ", decoded_data[i]);
       Log.verbose("\r\n");

       //here begins the attempt to compute the pre-compiled header.
       //we start with the header that has a payload size of zero
       u_int16_t payload_size = 200;  //let's define a 16 bit value for the payload size
       //now we want to put that into bit 7 of bytes 2 through 11, with the MSB to the left
       for (int i=2; i<12; i++)
       {
              il2p_header_precoded[i] |= ((payload_size >> (11-i)) & 0x01) << 7;
       }
       
       //it should now have the payload field updated
       Log.verbose(F("This is the updated header: \r\n"));
       for (int i=0; i<(header_size); i++) Log.verbose("%X, ", il2p_header_precoded[i]);

       //reuse the scrambled variable
       il2p_scramble_block(il2p_header_precoded, il2p_header_scrambled, header_size);

       Log.verbose(F("This is the scrambled header: \r\n"));
       for (int i=0; i<(header_size); i++) Log.verbose("%X, ", il2p_header_scrambled[i]);

       il2p_encode_rs(il2p_header_scrambled, header_size, 2, parity);
       for (int i=0; i<header_size; i++) decode_block[i]=il2p_header_scrambled[i];
       for (int i=header_size; i<(header_size+2); i++) decode_block[i] = parity[i-header_size];

       Log.verbose(F("Parity Bytes: \r\n"));
       for (int i=0; i< parity_size; i++) Log.trace("%i: %X\r\n", i, parity[i]); 

       Log.verbose(F("This is the complete header: \r\n"));
       for (int i=0; i<(header_size+2); i++) Log.verbose("%X, ", decode_block[i]);

}