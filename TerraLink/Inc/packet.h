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

#define MAX_PAYLOAD_SIZE 64
#define DATA_RECORD_SIZE sizeof(data_record_t)
#define CMD_RECORD_SIZE sizeof(cmd_record_t)

#define FLASH_MAX_NODES 5

#include "stdint.h"
#include "stdbool.h"
#include "flash_manage.h"
#include "STMDevice.h"

#pragma pack(push, 1)
typedef struct
{
	uint8_t dst_id;
	uint8_t src_id;
	uint8_t pkt_type;
	uint8_t seq;
	uint8_t len;
	uint16_t crc;
	uint8_t payload[];
} packet_t;
#pragma pack(pop)

#define PACKET_HEADER_SIZE (offsetof(packet_t, crc))
#define PACKET_SIZE(payload_len) (PACKET_HEADER_SIZE + CRC_SIZE + (payload_len))
#define PACKET_MAX_SIZE PACKET_SIZE(MAX_PAYLOAD_SIZE)
#define PACKET_TOTAL_SIZE(pkt) (PACKET_HEADER_SIZE + CRC_SIZE + (pkt)->len)

#pragma pack(push, 1)
typedef struct
{
	uint8_t type;
	uint32_t time_offset;
	uint32_t data;
} data_record_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	uint8_t type;
	uint16_t value;
} cmd_record_t;
#pragma pack(pop)

typedef enum
{
	COMMUNICATION,
	MEASUREMENT
} WAKEUP_TYPE;

#pragma pack(push, 1)
typedef struct
{
	WAKEUP_TYPE wakeup_type;
	uint16_t time;
} wakeup_t;
#pragma pack(pop)

enum
{
	PKT_REG_REQ = 0x01,
	PKT_ASSIGN_ID = 0x02,
	PKT_ACK = 0x03,
	PKT_DATA = 0x04,
	PKT_REQ_DATA = 0x05,
	PKT_REQ_ID = 0x06,
	PKT_TEST_CONN = 0x07,
	PKT_CMD = 0x08,
	PKT_CMD_DATA = 0x09,
	PKT_START = 0x0A,
	PKT_REQ_START = 0x0B, // Do zaimplementowania
	PKT_SYNC_CONFIG = 0x0C,
	PKT_INFO_NEXT_WAKEUP = 0x0D,
	PKT_DEVICE_INFO = 0x0E
};

typedef enum
{
	DATA_ID = 0x01,
	DATA_HANDSHAKE = 0x02,
	DATA_SOIL_MOISTURE = 0x03,
	DATA_LIGHT = 0x04,
	DATA_TEMP = 0x05
} DATA_TYPE;

// Packet memory management
packet_t *packet_alloc(uint8_t payload_len);
void packet_free(packet_t *pkt);
void packet_set_crc(packet_t *pkt);
uint16_t packet_get_crc(const packet_t *pkt);

uint16_t get_pkt_length(const packet_t *pkt);
uint8_t next_seq_number();
uint16_t crc16_compute(const uint8_t *data, uint16_t length);

uint8_t get_id(STM32_UID_t *uid);
uint8_t set_id(uint8_t current_id, uint8_t new_id);

uint8_t verify_pkt(packet_t *pkt);
uint8_t get_data(const packet_t *pkt, void *dataStructPtr, size_t dataStructSize);
uint8_t get_cmd_data(const packet_t *pkt, cmd_record_t *cmd_data);
uint8_t attach_data(packet_t *pkt, data_record_t *data);
uint8_t attach_cmd(packet_t *pkt, cmd_record_t *cmd);

packet_t *create_ack_pkt(const packet_t *received_pkt);
packet_t *create_handshake_response_pkt(const packet_t *req_pkt, const SDevice *argDevice);
packet_t *create_request_data_pkt(uint8_t dest_id, DATA_TYPE req_data_type);
packet_t *create_test_conn_pkt(uint8_t dest_id);
packet_t *create_sync_config_pkt(uint8_t dest_id);
packet_t *create_start_pkt(uint8_t dest_id);

// TODO: CRC Compute

#endif /* INC_PACKET_H_ */
