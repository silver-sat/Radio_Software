/**
 * @file testing_support.cpp
 * @author Tom Conrad (tom@silversat.org)
 * @brief testing support functions for AX5043 radio and Silversat radio board
 * @version 1.0.1
 * @date 2023-2-27

 */

 #include "testing_support.h"

 void printRegisters(ax_config& config){
    printf("Here are the AX5043 register contents \r\n");

  // this prints all but ADC, low power oscillator and DAC registers

  printf("registers below 0x70, general config \r\n");
  for (uint16_t reg = 0; reg < 0x70; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameters \r\n");
  // range 0x100 to 0x118
  for (uint16_t reg = 0x100; reg < 0x119; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 0 \r\n");
  // range 0x120 to 0x12F
  for (uint16_t reg = 0x120; reg < 0x130; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 1 \r\n");
  // range 0x130 to 0x13F
  for (uint16_t reg = 0x130; reg < 0x140; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 2 \r\n");
  // range 0x140 to 0x14F
  for (uint16_t reg = 0x140; reg < 0x150; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("receiver parameter set 3 \r\n");
  // range 0x150 to 0x15F
  for (uint16_t reg = 0x150; reg < 0x160; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("transmitter parameters \r\n");
  // range 0x160 to 0x171
  for (uint16_t reg = 0x160; reg < 0x172; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("pll and crystal parameters \r\n");
  // range 0x180 to 0x189
  for (uint16_t reg = 0x180; reg < 0x18A; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("packet format \r\n");
  // range 0x200 to 0x20B
  for (uint16_t reg = 0x200; reg < 0x20C; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("pattern match \r\n");
  // range 0x210 to 0x21E
  for (uint16_t reg = 0x210; reg < 0x21F; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("packet controller \r\n");
  // range 0x220 to 0x233
  for (uint16_t reg = 0x220; reg < 0x234; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("special functions \r\n");
  // 0x248, 24A, 250, 255
    printf("register= %2x | value= %2x \r\n", 0x248,
           ax_hw_read_register_8(&config, 0x248));
    printf("register= %2x | value= %2x \r\n", 0x24A,
           ax_hw_read_register_8(&config, 0x24A));
    printf("register= %2x | value= %2x \r\n", 0x250,
           ax_hw_read_register_8(&config, 0x250));
    printf("register= %2x | value= %2x \r\n", 0x255,
           ax_hw_read_register_8(&config, 0x255));

  printf("ADC & low power oscillator \r\n");
  // range 0x300 to 0x332
  for (uint16_t reg = 0x300; reg < 0x332; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }

  printf("peformance tuning registers \r\n");
  // range 0xF00 to 0xFFF
  for (uint16_t reg = 0xF00; reg <= 0xFFF; reg++) {
    printf("register= %2x | value= %2x \r\n", reg,
           ax_hw_read_register_8(&config, reg));
  }
}