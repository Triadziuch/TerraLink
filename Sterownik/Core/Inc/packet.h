/*
 * packet.h
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#ifndef INC_PACKET_H_
#define INC_PACKET_H_

#define HEADER_SIZE 5
#define CRC_SIZE 2

#define MAX_PAYLOAD_SIZE	15
#define DATA_RECORD_SIZE	5

#include "stdint.h"
#include "stdbool.h"
//#include "flash_manage.h"

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

typedef struct{
	uint8_t data_type;
	uint16_t data_time_offset;
	uint16_t data;
} data_record_t;

enum{
	PKT_REG_REQ		= 0x01,
	PKT_ASSIGN_ID	= 0x02,
	PKT_ACK			= 0x03,
	PKT_REQ_DATA	= 0x04,
	PKT_REQ_ID		= 0x05
};

enum{
	DATA_ID				= 0x01,
	DATA_SOIL_MOISTURE	= 0x02,
	DATA_LIGHT			= 0x03,
	DATA_DATA_TEMP		= 0x04
};

uint16_t get_pkt_length(const packet_t* pkt);
uint8_t next_seq_number();
uint16_t crc16_compute(const uint8_t *data, uint16_t length);
int verify_pkt(packet_t *pkt);
int get_data(const packet_t* pkt, uint8_t index, data_record_t* data);
int attach_data(packet_t* pkt, data_record_t* data);

//TODO: CRC Compute

#endif /* INC_PACKET_H_ */
