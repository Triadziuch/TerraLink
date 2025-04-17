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

//TODO: Flexible array
#pragma pack(push, 1)
typedef struct{
	uint8_t dst_id;
	uint8_t src_id;
	uint8_t pkt_type;
	uint8_t seq;
	uint8_t payload[MAX_PAYLOAD_SIZE];
	uint16_t crc16;
} packet_t;

enum{
	PKT_REG_REQ		= 0x01,
	PKT_ASSIGN_ID	= 0x02,
	PKT_ACK			= 0x03
};

//TODO: CRC Compute

#endif /* INC_PACKET_H_ */
