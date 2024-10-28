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

uint32_t IL2P_CRC::encode_crc(uint16_t CRC)
{
  uint8_t first_nibble = (CRC & 0x000F);
  uint8_t second_nibble = (CRC & 0x00F0)>>4;
  uint8_t third_nibble = (CRC & 0x0F00) >>8;
  uint8_t fourth_nibble = (CRC & 0xF000) >>12;

  uint32_t encoded_crc = (encode_table[fourth_nibble])<<24;
  encoded_crc |= (encode_table[third_nibble])<<16;
  encoded_crc |= (encode_table[second_nibble])<<8;
  encoded_crc |= encode_table[first_nibble];

  //Serial.print("encoded crc: ");Serial.println(encoded_crc, BIN);
  
  return (encoded_crc);
}

//this function is included in case the NinoTNC requires the CRC to be calculated over an AX.25 frame
//the bufffer is expected to hold the data to be encapsulate, no IL2P header
uint16_t IL2P_CRC::calculate_AX25(uint8_t *buf, int buf_length)
{
  uint8_t newbuff[216]; //create new buffer and add the ax25 header
  for (int i=0; i<16; i++) newbuff[i] = ax25_header[i];
  for (int i=0; i<buf_length; i++) newbuff[i+16] = buf[i];
  uint16_t crc = CRC16.ccitt(newbuff, buf_length+16);
  return crc;
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
  //could just shortcut all of the above and get to here..but you definitiely need to run the decode table.
  uint16_t new_crc = (corrected_fourth_byte&0x0F)<<12 | (corrected_third_byte&0x0F)<<8 | (corrected_second_byte&0x0F)<<4 | (corrected_first_byte)&0x0F;

  //Serial.print("new_crc: ");Serial.println(new_crc, BIN);
  Log.verbose(F("Rcvd Tx CRC: %X\r\n"), new_crc);

  m_crc = CRC16.ccitt(buf, buf_length);
  Log.notice(F("Rx CRC: %X\r\n"), m_crc);

  if (new_crc == m_crc) return true;

  return false;
}


uint16_t IL2P_CRC::extract_crc(uint32_t received_crc)
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
  Log.verbose(F("Rcvd Tx CRC: %X\r\n"), new_crc);

  return new_crc;
}
