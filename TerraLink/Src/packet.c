/*
 * packet.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "packet.h"
#include "comm.h"
#include "NodeManage.h"
#include "Packet_impl.h"
#include <stdlib.h>
#include <string.h>

// Allocate packet with specified payload length
packet_t *packet_alloc(uint8_t payload_len)
{
	if (payload_len > MAX_PAYLOAD_SIZE)
		return NULL;

	size_t alloc_size = PACKET_SIZE(payload_len);
	packet_t *pkt = (packet_t *)malloc(alloc_size);
	if (pkt != NULL)
	{
		memset(pkt, 0, alloc_size);
	}
	return pkt;
}

// Free allocated packet
void packet_free(packet_t *pkt)
{
	if (pkt != NULL)
	{
		free(pkt);
	}
}

// Set CRC - compute from header and payload, store in crc field
void packet_set_crc(packet_t *pkt)
{
	if (pkt == NULL)
		return;

	// Compute CRC from header (5 bytes) + payload
	uint8_t temp_buffer[PACKET_MAX_SIZE];
	memcpy(temp_buffer, pkt, PACKET_HEADER_SIZE);
	memcpy(temp_buffer + PACKET_HEADER_SIZE, pkt->payload, pkt->len);

	uint16_t crc = crc16_compute(temp_buffer, PACKET_HEADER_SIZE + pkt->len);
	pkt->crc = crc;
}

// Get CRC from packet
uint16_t packet_get_crc(const packet_t *pkt)
{
	if (pkt == NULL)
		return 0;

	return pkt->crc;
}

uint16_t get_pkt_length(const packet_t *pkt)
{
	return PACKET_TOTAL_SIZE(pkt);
}

uint8_t next_seq_number()
{
	static uint8_t seq = 0;
	return seq++;
}

uint16_t crc16_compute(const uint8_t *data, uint16_t length)
{
	uint32_t crc_result;

	crc_result = HAL_CRC_Calculate(&hcrc, (uint32_t *)data, length);

	return (uint16_t)(crc_result & 0xFFFF);
}

uint8_t verify_pkt(packet_t *pkt)
{

	uint8_t *rx_buf = lora_buffer;
	if (rx_buf == NULL || pkt == NULL)
		return 0;

	uint8_t payload_len = rx_buf[4];
	if (payload_len > MAX_PAYLOAD_SIZE)
		return 0;

	// W rx_buf: header (5), payload (len), crc (2)
	// CRC jest na końcu odbieranych danych
	uint16_t received_crc;
	memcpy(&received_crc, rx_buf + PACKET_HEADER_SIZE + payload_len, CRC_SIZE);

	// Oblicz CRC z headera i payload
	uint16_t computed_crc = crc16_compute(rx_buf, PACKET_HEADER_SIZE + payload_len);

	if (computed_crc != received_crc)
		return 0;

	// Copy header (5 bytes)
	memcpy(pkt, rx_buf, PACKET_HEADER_SIZE);
	// Copy CRC to structure
	pkt->crc = received_crc;
	// Copy payload if exists
	if (payload_len > 0)
		memcpy(pkt->payload, rx_buf + PACKET_HEADER_SIZE, payload_len);

	comm_print_pkt(pkt, "Odebrano pakiet");
	return 1;
}

uint8_t get_data(const packet_t *pkt, void *dataStructPtr, size_t dataStructSize)
{
	if (pkt == NULL || dataStructPtr == NULL || pkt->len != dataStructSize)
		return 0;

	memcpy(dataStructPtr, pkt->payload, pkt->len);
	return 1;
}

uint8_t get_cmd_data(const packet_t *pkt, cmd_record_t *cmd_data)
{
	if (pkt == NULL || cmd_data == NULL || pkt->len < CMD_RECORD_SIZE)
		return 0;

	memcpy(cmd_data, pkt->payload, CMD_RECORD_SIZE);

	return 1;
}

uint8_t attach_data(packet_t *pkt, data_record_t *data)
{
	if (pkt == NULL || data == NULL)
		return 0;

	if (pkt->len + DATA_RECORD_SIZE > MAX_PAYLOAD_SIZE)
		return 0;

	memcpy(pkt->payload + pkt->len, data, DATA_RECORD_SIZE);
	pkt->len += DATA_RECORD_SIZE;

	return 1;
}

uint8_t attach_cmd(packet_t *pkt, cmd_record_t *cmd)
{
	if (pkt == NULL || cmd == NULL)
		return 0;

	memcpy(pkt->payload + pkt->len, cmd, CMD_RECORD_SIZE);
	pkt->len += CMD_RECORD_SIZE;

	return 1;
}

// uint8_t set_id(uint8_t current_id, uint8_t new_id){
//	for (int i = 0; i < UIDs_count; ++i)
//		if (UIDs_assigned_ID[i] == current_id){
//			UIDs_assigned_ID[i] = new_id;
//			return 1;
//		}
//	return 0;
// }

uint8_t create_ack_pkt(packet_t *ack_pkt, const packet_t *received_pkt)
{
	if (received_pkt == NULL || ack_pkt == NULL)
		return 0;

	ack_pkt->dst_id = received_pkt->src_id;
	ack_pkt->src_id = get_own_id();
	ack_pkt->pkt_type = PKT_ACK;
	ack_pkt->seq = received_pkt->seq + 1;
	ack_pkt->len = 0;
	packet_set_crc(ack_pkt);

	return 1;
}

packet_t *create_handshake_response_pkt(const packet_t *req_pkt, const SDevice* argDevice)
{
	if (req_pkt == NULL || argDevice == NULL)
		return NULL;

	packet_t *resp_pkt = packet_alloc(sizeof(SDevice));
	if (resp_pkt == NULL)
		return NULL;
	memcpy(resp_pkt->payload, argDevice, sizeof(SDevice));

	resp_pkt->dst_id = req_pkt->src_id;
	resp_pkt->src_id = get_own_id();
	resp_pkt->pkt_type = PKT_ASSIGN_ID;
	resp_pkt->seq = req_pkt->seq + 1;
	resp_pkt->len = sizeof(SDevice);
	packet_set_crc(resp_pkt);

	return resp_pkt;
}

uint8_t create_request_data_pkt(packet_t *req_pkt, uint8_t dest_id,
								DATA_TYPE req_data_type)
{
	if (req_pkt == NULL)
		return 0;

	data_record_t req_data_record;
	req_data_record.type = req_data_type;
	req_data_record.time_offset = 0;
	req_data_record.data = 0;

	req_pkt->dst_id = dest_id;
	req_pkt->src_id = get_own_id();
	req_pkt->pkt_type = PKT_REQ_DATA;
	req_pkt->seq = next_seq_number();
	req_pkt->len = 0;
	if (!attach_data(req_pkt, &req_data_record))
		return 0;
	packet_set_crc(req_pkt);

	return 1;
}

uint8_t create_test_conn_pkt(packet_t *test_pkt, uint8_t dest_id)
{
	if (test_pkt == NULL)
		return 0;

	test_pkt->dst_id = dest_id;
	test_pkt->src_id = get_own_id();
	test_pkt->pkt_type = PKT_TEST_CONN;
	test_pkt->seq = next_seq_number();
	test_pkt->len = 0;
	packet_set_crc(test_pkt);

	return 1;
}

packet_t *create_sync_config_pkt(uint8_t dest_id)
{
	SNodeConfig *node_config = get_node_config(dest_id);
	if (node_config == NULL)
		return NULL;

	packet_t *pkt = packet_alloc(sizeof(SNodeConfig));
	if (pkt == NULL)
		return NULL;

	pkt->dst_id = dest_id;
	pkt->src_id = get_own_id();
	pkt->pkt_type = PKT_SYNC_CONFIG;
	pkt->seq = next_seq_number();
	pkt->len = sizeof(SNodeConfig);
	memcpy(pkt->payload, node_config, sizeof(SNodeConfig));
	packet_set_crc(pkt);

	return pkt;
}

uint8_t create_cmd_pkt(packet_t *cmd_pkt, uint8_t dest_id, CMD_TYPE cmd,
					   uint16_t value)
{
	if (cmd_pkt == NULL)
		return 0;

	cmd_pkt->dst_id = dest_id;
	cmd_pkt->src_id = get_own_id();
	cmd_pkt->pkt_type = PKT_CMD;
	cmd_pkt->seq = next_seq_number();
	cmd_pkt->len = 0;

	// If set command
	if (cmd % 2 == 0)
	{
		if (cmd != CMD_SET_COMM_WAKEUP_TIMER_INTERVAL && cmd != CMD_SET_MEASUREMENT_WAKEUP_TIMER_INTERVAL)
			if (value > 255)
				return 0;

		cmd_record_t cmd_record;
		cmd_record.type = cmd;
		cmd_record.value = value;

		if (!attach_cmd(cmd_pkt, &cmd_record))
			return 0;
	}
	else
	{
		cmd_record_t cmd_record;
		cmd_record.type = cmd;
		cmd_record.value = 0;

		if (!attach_cmd(cmd_pkt, &cmd_record))
			return 0;
	}

	packet_set_crc(cmd_pkt);

	return 1;
}

packet_t *create_start_pkt(uint8_t dest_id)
{
	packet_t *start_pkt = packet_alloc(0);
	if (start_pkt == NULL)
		return NULL;

	start_pkt->dst_id = dest_id;
	start_pkt->src_id = get_own_id();
	start_pkt->pkt_type = PKT_START;
	start_pkt->seq = next_seq_number();
	start_pkt->len = 0;
	packet_set_crc(start_pkt);

	return start_pkt;
}
