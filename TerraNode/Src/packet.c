/*
 * packet.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "packet.h"
#include "comm.h"
#include <stdlib.h>
#include <string.h>
#include "STMDevice.h"

SNodeConfig node_config = {
	.comm_wakeup_timer_interval = 0,
	.measurement_wakeup_timer_interval = 0,
	.comm_wakeup_timer_time_awake = 0,
	.measurement_wakeup_timer_time_awake = 0
};

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

uint16_t get_pkt_length(const packet_t *pkt) {
	return PACKET_TOTAL_SIZE(pkt);
}

uint8_t next_seq_number() {
	static uint8_t seq = 0;
	return seq++;
}

uint16_t crc16_compute(const uint8_t *data, uint16_t length) {
	uint32_t crc_result;

	crc_result = HAL_CRC_Calculate(&hcrc, (uint32_t*) data, length);

	return (uint16_t) (crc_result & 0xFFFF);
}

uint8_t verify_pkt(packet_t *pkt) {

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

	return 1;
}

uint8_t get_data(const packet_t *pkt, void *dataStructPtr, size_t dataStructSize)
{
	if (pkt == NULL || dataStructPtr == NULL || pkt->len != dataStructSize)
		return 0;

	memcpy(dataStructPtr, pkt->payload, pkt->len);
	return 1;
}

uint8_t get_cmd_data(const packet_t *pkt, cmd_record_t *cmd_data) {
	if (pkt == NULL || cmd_data == NULL)
		return 0;

	memcpy(cmd_data, pkt->payload, CMD_RECORD_SIZE);

	return 1;
}

uint8_t attach_data(packet_t *pkt, data_record_t *data) {
	if (pkt == NULL || data == NULL)
		return 0;

	if (pkt->len + DATA_RECORD_SIZE > MAX_PAYLOAD_SIZE)
		return 0;

	memcpy(pkt->payload + pkt->len, data, DATA_RECORD_SIZE);
	pkt->len += DATA_RECORD_SIZE;

	return 1;
}

uint8_t attach_cmd(packet_t *pkt, cmd_record_t *cmd) {
	if (pkt == NULL || cmd == NULL)
		return 0;

	memcpy(pkt->payload + pkt->len, cmd, CMD_RECORD_SIZE);
	pkt->len += CMD_RECORD_SIZE;

	return 1;
}

uint8_t create_ack_pkt(packet_t *ack_pkt, const packet_t *received_pkt) {
	if (received_pkt == NULL || ack_pkt == NULL)
		return 0;

	ack_pkt->dst_id = received_pkt->src_id;
	ack_pkt->src_id = FLASH_NODE_ID_get();
	ack_pkt->pkt_type = PKT_ACK;
	ack_pkt->seq = received_pkt->seq + 1;
	ack_pkt->len = 0;
	packet_set_crc(ack_pkt);

	return 1;
}

uint8_t create_handshake_pkt(packet_t *pkt) {
	if (pkt == NULL)
		return 0;

	pkt->dst_id = BROADCAST_ID;
	pkt->src_id = 0;
	pkt->pkt_type = PKT_REG_REQ;
	pkt->seq = next_seq_number();
	pkt->len = sizeof(SDeviceUID);

	SDeviceUID stm32_uid;
	FLASH_NODE_UID_get(&stm32_uid);
	memcpy(pkt->payload, &stm32_uid, sizeof(SDeviceUID));
	packet_set_crc(pkt);

	return 1;
}

uint8_t create_data_pkt(packet_t *data_pkt, const packet_t *received_pkt) {
	if (data_pkt == NULL)
		return 0;

	data_pkt->src_id = FLASH_NODE_ID_get();
	data_pkt->pkt_type = PKT_DATA;

	if (received_pkt == NULL) {
		data_pkt->dst_id = HIVE_ID;
		data_pkt->seq = next_seq_number();
	} else {
		data_pkt->dst_id = received_pkt->src_id;
		data_pkt->seq = received_pkt->seq + 1;
	}

	data_pkt->len = 0;

	return 1;
}
