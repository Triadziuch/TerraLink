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
#define CMD_RECORD_SIZE		3

#define HIVE_ID	96

#include "stdint.h"
#include "stdbool.h"
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

#pragma pack(push, 1)
typedef struct{
	uint8_t type;
	uint16_t value;
} cmd_record_t;
#pragma pack(pop)

enum{
	PKT_REG_REQ		= 0x01,
	PKT_ASSIGN_ID	= 0x02,
	PKT_ACK			= 0x03,
	PKT_DATA		= 0x04,
	PKT_REQ_DATA	= 0x05,
	PKT_REQ_ID		= 0x06,
	PKT_TEST_CONN	= 0x07,
	PKT_CMD			= 0x08,
	PKT_CMD_DATA	= 0x09
};

typedef enum {
	DATA_ID				= 0x01,
	DATA_HANDSHAKE		= 0x02,
	DATA_SOIL_MOISTURE	= 0x03,
	DATA_LIGHT			= 0x04,
	DATA_TEMP			= 0x05
} DATA_TYPE;

typedef enum{
	CMD_GET_COMM_WAKEUP_TIMER_INTERVAL			= 0x01,
	CMD_SET_COMM_WAKEUP_TIMER_INTERVAL			= 0x02,
	CMD_GET_COMM_WAKEUP_TIMER_TIME_AWAKE		= 0x03,
	CMD_SET_COMM_WAKEUP_TIMER_TIME_AWAKE		= 0x04,
	CMD_GET_MEASUREMENT_WAKEUP_TIMER_INTERVAL	= 0x05,
	CMD_SET_MEASUREMENT_WAKEUP_TIMER_INTERVAL	= 0x06,
	CMD_GET_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE	= 0x07,
	CMD_SET_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE	= 0x08,
	CMD_GET_HIVE_ID	= 0x09,
	CMD_SET_HIVE_ID	= 0x0A,
	CMD_GET_NODE_ID	= 0x0B,
	CMD_SET_NODE_ID = 0x0C
} CMD_TYPE;

uint16_t get_pkt_length(const packet_t* pkt);
uint8_t next_seq_number();
uint16_t crc16_compute(const uint8_t *data, uint16_t length);

uint8_t get_id(STM32_UID_t* uid);
uint8_t id_exists(uint8_t link_id);
uint8_t set_id(uint8_t current_id, uint8_t new_id);

uint8_t verify_pkt(packet_t *pkt);
uint8_t get_data(const packet_t* pkt, uint8_t index, data_record_t* data);
uint8_t get_cmd_data(const packet_t* pkt, cmd_record_t* cmd_data);
uint8_t attach_data(packet_t* pkt, data_record_t* data);
uint8_t attach_cmd(packet_t* pkt, cmd_record_t* cmd);

uint8_t create_ack_pkt(packet_t* ack_pkt, const packet_t *received_pkt);
uint8_t create_handshake_response_pkt(packet_t *resp_pkt, const packet_t *req_pkt);
uint8_t create_request_data_pkt(packet_t* req_pkt, uint8_t dest_id, DATA_TYPE req_data_type);
uint8_t create_test_conn_pkt(packet_t *test_pkt, uint8_t dest_id);
uint8_t create_cmd_pkt(packet_t *cmd_pkt, uint8_t dest_id, CMD_TYPE cmd, uint16_t value);

//TODO: CRC Compute

#endif /* INC_PACKET_H_ */
