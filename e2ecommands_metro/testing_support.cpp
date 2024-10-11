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
    Log.verbose("Here are the AX5043 register contents \r\n");

    // this prints all but ADC, low power oscillator and DAC registers

    Log.verbose("registers below 0x70, general config \r\n");
    for (uint16_t reg = 0; reg < 0x70; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("receiver parameters \r\n");
    // range 0x100 to 0x118
    for (uint16_t reg = 0x100; reg < 0x119; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("receiver parameter set 0 \r\n");
    // range 0x120 to 0x12F
    for (uint16_t reg = 0x120; reg < 0x130; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("receiver parameter set 1 \r\n");
    // range 0x130 to 0x13F
    for (uint16_t reg = 0x130; reg < 0x140; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("receiver parameter set 2 \r\n");
    // range 0x140 to 0x14F
    for (uint16_t reg = 0x140; reg < 0x150; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("receiver parameter set 3 \r\n");
    // range 0x150 to 0x15F
    for (uint16_t reg = 0x150; reg < 0x160; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("transmitter parameters \r\n");
    // range 0x160 to 0x171
    for (uint16_t reg = 0x160; reg < 0x172; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("pll and crystal parameters \r\n");
    // range 0x180 to 0x189
    for (uint16_t reg = 0x180; reg < 0x18A; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("packet format \r\n");
    // range 0x200 to 0x20B
    for (uint16_t reg = 0x200; reg < 0x20C; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("pattern match \r\n");
    // range 0x210 to 0x21E
    for (uint16_t reg = 0x210; reg < 0x21F; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("packet controller \r\n");
    // range 0x220 to 0x233
    for (uint16_t reg = 0x220; reg < 0x234; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }

    Log.verbose("special functions \r\n");
    // 0x248, 24A, 250, 255
    Log.verbose("register= %2x | value= %2x \r\n", 0x248, 
           radio.getRegValue(0x248));
    Log.verbose("register= %2x | value= %2x \r\n", 0x24A,
           radio.getRegValue(0x24A));
    Log.verbose("register= %2x | value= %2x \r\n", 0x250,
           radio.getRegValue(0x250));
    Log.verbose("register= %2x | value= %2x \r\n", 0x255,
           radio.getRegValue(0x255));

    Log.verbose("ADC & low power oscillator \r\n");
    // range 0x300 to 0x332
    for (uint16_t reg = 0x300; reg < 0x332; reg++)
    {
        Log.verbose("register= %2x | value= %2x \r\n", reg,
               radio.getRegValue(reg));
    }
    // On semi recommends not touching these..even reading from them.
    /*
    Log.verbose("peformance tuning registers \r\n");
    // range 0xF00 to 0xFFF
    for (uint16_t reg = 0xF00; reg <= 0xFFF; reg++)
    {
           Log.verbose("register= %2x | value= %2x \r\n", reg,
           radio.getRegValue(reg));
    }
    */
}

void efuseTesting(Efuse &efuse, ExternalWatchdog &watchdog)
{
    while (true)
    {
        int start_time = millis();
        Log.verbose("current: %5f \r\n", efuse.measure_current());
        Log.verbose("execution time: %lx \r\n", millis() - start_time);
        watchdog.trigger();
        efuse.overcurrent(true);
        // efuse.overcurrent(false);
        delay(1000); // run once per second
    }
}

void il2p_testing()
{
       unsigned char il2p_header_raw_example[13]{0x63, 0xF1, 0x40, 0x40, 0x40, 0x0, 0x6B, 0x2B, 0x54, 0x28, 0x25, 0x2A, 0x0F};
       unsigned char il2p_header_scrambled[13];
       int header_size = sizeof(il2p_header_raw_example);
       il2p_scramble_block (il2p_header_raw_example, il2p_header_scrambled, sizeof(il2p_header_raw_example));
       Serial.println("Scrambled Header: ");

       for (int i=0; i < header_size; i++)
       {
       
       Log.trace("header: %X \r\n", il2p_header_scrambled[i]);
       }
       //Log.verbose("\r\n");

       il2p_init();
       unsigned char parity[2];
       void il2p_encode_rs (unsigned char *tx_data, int data_size, int num_parity, unsigned char *parity_out);
       il2p_encode_rs(il2p_header_scrambled, header_size, 2, parity);

       Log.trace("P0: %X\r\n", parity[0]); 
       Log.trace("P1: %X\r\n", parity[1]);

       //int il2p_decode_rs (unsigned char *rec_block, int data_size, int num_parity, unsigned char *out)
       unsigned char decode_block[15];
       
       il2p_decode_rs(decode_block, 13, 2, decoded_data);
}