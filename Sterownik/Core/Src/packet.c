/*
 * packet.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "packet.h"
#include "comm.h"

uint16_t get_pkt_length(const packet_t* pkt){
	return HEADER_SIZE + pkt->len + CRC_SIZE;
}

uint8_t next_seq_number(){
	static uint8_t seq = 0;
	return seq++;
}

uint16_t crc16_compute(const uint8_t *data, uint16_t length){
	return 77;
}
