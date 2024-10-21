#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\il2p_rs.h"
/**
* @file il2p_rs.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library for provide il2p support
* @version 1.0.1
* @date 2024-10-8

*/

#ifndef IL2P_RS_H
#define IL2P_RS_H

void il2p_init();
void il2p_encode_rs (unsigned char *tx_data, int data_size, int num_parity, unsigned char *parity_out);
int  il2p_decode_rs (unsigned char *rec_block, int data_size, int num_parity, unsigned char *out);


#endif