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
	return 1;
}

bool comm_test_pkt(void){
	packet_t req;
	req.dst_id = 1;
	req.src_id = FLASH_NODE_ID_get();
	req.pkt_type = PKT_REG_REQ;
	req.seq = next_seq_number();
	req.len = 0;
	req.crc16 = crc16_compute((uint8_t*) &req, sizeof(packet_t) - 2);

	while(1){
		HAL_NVIC_DisableIRQ(EXTI_LINE);
		lora_data_ready = false;

		HAL_Delay(10);

	}
	comm_send(&req);
}

bool handshake(void){
	packet_t req;
	req.dst_id = 1;
	req.src_id = 69;//FLASH_NODE_ID_get();
	req.pkt_type = PKT_REG_REQ;
	req.seq = next_seq_number();
	req.len = 0;
	req.crc16 = crc16_compute((uint8_t*) &req, sizeof(packet_t) - 2);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt){

	}
}
