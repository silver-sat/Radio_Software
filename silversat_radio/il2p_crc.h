/**
* @file il2p_crc.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library that calculates the il2p hamming coded CRC bytes and verifies the CRC for a buffer
*     based on the received CRC
* @version 1.0.1
* @date 2024-10-13

 il2p_crc.h - Library that calculates the il2p hamming coded CRC bytes and verifies the CRC for a buffer
Created by Tom Conrad, July 17, 2024.
Released into the public domain.


*/

#ifndef IL2P_CRC_H
#define IL2P_CRC_H

#include <FastCRC.h>
#include <ArduinoLog.h>

class IL2P_CRC{
  public:
  IL2P_CRC();
  uint32_t calculate(uint8_t *buff, int buf_length);
  uint32_t encode_crc(uint16_t CRC);  //given a 16 bit unencoded crc, calculate the 32 bit encoded one
  uint16_t calculate_AX25(uint8_t *buf, int buf_length);  //calculate the CRC for an ax.25 packet
  bool verify(uint8_t *buf, int buf_length, uint32_t received_crc);
  uint16_t extract_crc(uint32_t received_crc); //given a 32 bit encoded crc, extract the 16 bit unencoded one
  
  private:
  uint8_t encode_table[16] = { 0x0, 0x71, 0x62, 0x13, 0x54, 0x25, 0x36, 0x47, 0x38, 0x49, 0x5a, 0x2b, 0x6c, 0x1d, 0x0e, 0x7f };

  uint8_t decode_table[128] = {
  0x0, 0x0, 0x0, 0x3, 0x0, 0x5, 0xe, 0x7,
  0x0, 0x9, 0xe, 0xb, 0xe, 0xd, 0xe, 0xe,
  0x0, 0x3, 0x3, 0x3, 0x4, 0xd, 0x6, 0x3,
  0x8, 0xd, 0xa, 0x3, 0xd, 0xd, 0xe, 0xd,
  0x0, 0x5, 0x2, 0xb, 0x5, 0x5, 0x6, 0x5,
  0x8, 0xb, 0xb, 0xb, 0xc, 0x5, 0xe, 0xb,
  0x8, 0x1, 0x6, 0x3, 0x6, 0x5, 0x6, 0x6,
  0x8, 0x8, 0x8, 0xb, 0x8, 0xd, 0x6, 0xf,
  0x0, 0x9, 0x2, 0x7, 0x4, 0x7, 0x7, 0x7,
  0x9, 0x9, 0xa, 0x9, 0xc, 0x9, 0xe, 0x7,
  0x4, 0x1, 0xa, 0x3, 0x4, 0x4, 0x4, 0x7,
  0xa, 0x9, 0xa, 0xa, 0x4, 0xd, 0xa, 0xf,
  0x2, 0x1, 0x2, 0x2, 0xc, 0x5, 0x2, 0x7,
  0xc, 0x9, 0x2, 0xb, 0xc, 0xc, 0xc, 0xf,
  0x1, 0x1, 0x2, 0x1, 0x4, 0x1, 0x6, 0xf,
  0x8, 0x1, 0xa, 0xf, 0xc, 0xf, 0xf, 0xf };

  /*  Silversat Club call sign
  uint8_t ax25_header[16] = 
  {
    0x96, 0x86, 0x66, 0xAC, 0xAC, 0xAE, 0x00, //seven byte Destination address, SSID = 0
    0x96, 0x86, 0x66, 0xAC, 0xAC, 0xAE, 0x01, //seven byte Source Address, SSID = 1
    0x03, 0xF0 //control, PID
  };
  */

  //  FCC call sign
  uint8_t ax25_header[16] = 
  {
    0xAE, 0xA0, 0x64, 0xB0, 0x8E, 0xAE, 0x00,
    0xAE, 0xA0, 0x64, 0xB0, 0x8E, 0xAE, 0x01,
    0x03, 0xF0
  };

  uint16_t m_crc;

  uint32_t m_coded_crc;

  FastCRC16 CRC16;
  
};

#endif
