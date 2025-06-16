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

#define MAX_PAYLOAD_SIZE	27
#define DATA_RECORD_SIZE	9

#define BROADCAST_ID	255

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

#pragma pack(push, 1)
typedef struct{
	uint8_t type;
	uint32_t time_offset;
	uint32_t data;
} data_record_t;
#pragma pack(pop)

enum{
	PKT_REG_REQ		= 0x01,
	PKT_ASSIGN_ID	= 0x02,
	PKT_ACK			= 0x03,
	PKT_DATA		= 0x04,
	PKT_REQ_DATA	= 0x05,
	PKT_REQ_ID		= 0x06,
	PKT_TEST_CONN	= 0x07
};

typedef enum {
	DATA_ID				= 0x01,
	DATA_HANDSHAKE		= 0x02,
	DATA_SOIL_MOISTURE	= 0x03,
	DATA_LIGHT			= 0x04,
	DATA_TEMP			= 0x05
} DATA_TYPE;

uint16_t get_pkt_length(const packet_t* pkt);
uint8_t next_seq_number();
uint16_t crc16_compute(const uint8_t *data, uint16_t length);

uint8_t verify_pkt(packet_t *pkt);
uint8_t get_data(const packet_t* pkt, uint8_t index, data_record_t* data);
uint8_t attach_data(packet_t* pkt, data_record_t* data);

uint8_t create_ack_pkt(packet_t *ack_pkt, const packet_t *received_pkt);
uint8_t create_handshake_pkt(packet_t* pkt);
uint8_t create_data_pkt(packet_t *data_pkt, const packet_t *received_pkt);

//TODO: CRC Compute

#endif /* INC_PACKET_H_ */
