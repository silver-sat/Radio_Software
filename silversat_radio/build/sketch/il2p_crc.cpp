#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\il2p_crc.cpp"
/**
* @file il2p_crc.cpp
* @author Tom Conrad (tom@silversat.org)
* @brief Library that calculates the il2p hamming coded CRC bytes and verifies the CRC for a buffer
*     based on the received CRC
* @version 1.0.1
* @date 2024-10-13

 il2p_crc.h - Library that calculates the il2p hamming coded CRC bytes and verifies the CRC for a buffer
Created by Tom Conrad, July 17, 2024.
Released into the public domain.


*/

#include "il2p_crc.h"

IL2P_CRC::IL2P_CRC()
{
  
  
}

uint32_t IL2P_CRC::calculate(uint8_t *buf, int buf_length)
{
  uint16_t m_crc = CRC16.ccitt(buf, buf_length);
  //Serial.print("crc for buff: ");Serial.println(m_crc, BIN);
  uint8_t first_nibble = (m_crc & 0x000F);
  uint8_t second_nibble = (m_crc & 0x00F0)>>4;
  uint8_t third_nibble = (m_crc & 0x0F00) >>8;
  uint8_t fourth_nibble = (m_crc & 0xF000) >>12;

  uint32_t encoded_crc = (encode_table[fourth_nibble])<<24;
  encoded_crc |= (encode_table[third_nibble])<<16;
  encoded_crc |= (encode_table[second_nibble])<<8;
  encoded_crc |= encode_table[first_nibble];

  //Serial.print("encoded crc: ");Serial.println(encoded_crc, BIN);
  
  return (encoded_crc);
}

bool IL2P_CRC::verify(uint8_t *buf, int buf_length, uint32_t received_crc)
{
  //decode the received CRC
  uint8_t corrected_fourth_byte = decode_table[(received_crc & 0xFF000000)>>24];
  uint8_t corrected_third_byte  = decode_table[(received_crc & 0x00FF0000)>>16];
  uint8_t corrected_second_byte = decode_table[(received_crc & 0x0000FF00)>>8];
  uint8_t corrected_first_byte  = decode_table[ received_crc & 0x000000FF];

  uint32_t corrected_crc = (corrected_fourth_byte<<24) | (corrected_third_byte<<16) | (corrected_second_byte<<8)| corrected_first_byte;

  //Serial.print("corrected crc: ");Serial.println(corrected_crc, BIN);

  uint16_t new_crc = (corrected_fourth_byte&0x0F)<<12 | (corrected_third_byte&0x0F)<<8 | (corrected_second_byte&0x0F)<<4 | (corrected_first_byte)&0x0F;

  //Serial.print("new_crc: ");Serial.println(new_crc, BIN);
  Log.verbose(F("received CRC: %X\r\n"), new_crc);

  m_crc = CRC16.ccitt(buf, buf_length);
  Log.verbose(F("buffer CRC: %X\r\n"), m_crc);

  if (new_crc == m_crc) return true;

  return false;
}
