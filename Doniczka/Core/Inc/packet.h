/*
 * packet.h
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#ifndef INC_PACKET_H_
#define INC_PACKET_H_

#define MAX_PAYLOAD_SIZE	16

#include "stdint.h"
#include "flash_manage.h"

//TODO: Flexible array
#pragma pack(push, 1)
typedef struct{
	uint8_t dst_id;
	uint8_t src_id;
	uint8_t pkt_type;
	uint8_t seq;
	uint8_t len;
	uint8_t payload[MAX_PAYLOAD_SIZE];
	uint16_t crc16;
} packet_t;
#pragma pack(pop)

enum{
	PKT_REG_REQ		= 0x01,
	PKT_ASSIGN_ID	= 0x02,
	PKT_ACK			= 0x03
};

uint16_t get_pkt_length(const packet_t* pkt);
uint8_t next_seq_number();
uint16_t crc16_compute(const uint8_t *data, uint16_t length);
bool comm_test_pkt(void);


//TODO: CRC Compute

#endif /* INC_PACKET_H_ */
