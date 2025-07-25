/*
 * packet.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "packet.h"
#include "comm.h"

uint16_t get_pkt_length(const packet_t *pkt) {
	return HEADER_SIZE + pkt->len + CRC_SIZE;
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
	if (rx_buf == NULL)
		return false;

	uint8_t payload_len = rx_buf[4];
	if (payload_len > MAX_PAYLOAD_SIZE)
		return false;

	uint8_t actual_len = HEADER_SIZE + payload_len + CRC_SIZE;
	uint16_t received_crc;
	memcpy(&received_crc, rx_buf + actual_len - CRC_SIZE, CRC_SIZE);

	uint16_t computed_crc = crc16_compute(rx_buf, actual_len - CRC_SIZE);

	if (computed_crc != received_crc)
		return false;

	memcpy(pkt, rx_buf, HEADER_SIZE);
	if (payload_len > 0)
		memcpy(pkt->payload, rx_buf + HEADER_SIZE, payload_len);
	memcpy(&pkt->crc16, rx_buf + HEADER_SIZE + payload_len, CRC_SIZE);

	return true;
}

uint8_t get_data(const packet_t *pkt, uint8_t index, data_record_t *data) {
	if (pkt == NULL || data == NULL)
		return 0;

	uint8_t total_records = pkt->len / DATA_RECORD_SIZE;
	if (total_records <= index)
		return 0;

	uint8_t offset = index * DATA_RECORD_SIZE;

	memcpy(data, pkt->payload + offset, DATA_RECORD_SIZE);

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
	ack_pkt->crc16 = crc16_compute((uint8_t*) ack_pkt,
			get_pkt_length(ack_pkt) - CRC_SIZE);

	return 1;
}

uint8_t create_handshake_pkt(packet_t *pkt) {
	if (pkt == NULL)
		return 0;

	pkt->dst_id = BROADCAST_ID;
	pkt->src_id = 0;
	pkt->pkt_type = PKT_REG_REQ;
	pkt->seq = next_seq_number();
	pkt->len = 0;

	STM32_UID_t stm32_uid;
	FLASH_NODE_UID_get(&stm32_uid);

	data_record_t handshake_data[3];

	handshake_data[0].data = stm32_uid.UID_0;
	handshake_data[1].data = stm32_uid.UID_1;
	handshake_data[2].data = stm32_uid.UID_2;

	for (int i = 0; i < 3; ++i) {
		handshake_data[i].type = DATA_HANDSHAKE;
		handshake_data[i].time_offset = 0;

		if (!attach_data(pkt, &handshake_data[i]))
			return 0;
	}

	pkt->crc16 = crc16_compute((uint8_t*) pkt, get_pkt_length(pkt) - CRC_SIZE);

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

uint8_t create_cmd_data_resp_pkt(packet_t *cmd_data_pkt,
		const packet_t *received_pkt) {
	if (!cmd_data_pkt || !received_pkt)
		return 0;

	cmd_record_t cmd_data;
	if (!get_cmd_data(received_pkt, &cmd_data))
		return 0;

	cmd_data_pkt->dst_id = FLASH_HIVE_ID_get();
	cmd_data_pkt->src_id = FLASH_NODE_ID_get();
	cmd_data_pkt->pkt_type = PKT_CMD_DATA;
	cmd_data_pkt->seq = received_pkt->seq + 1;
	cmd_data_pkt->len = 0;

	switch (cmd_data.type) {
	case CMD_SET_COMM_WAKEUP_TIMER_INTERVAL:
		if (!FLASH_COMM_WAKEUP_TIMER_INTERVAL_set(cmd_data.value))
			return 0;
	case CMD_GET_COMM_WAKEUP_TIMER_INTERVAL:
		cmd_data.value = FLASH_COMM_WAKEUP_TIMER_INTERVAL_get();
		break;

	case CMD_SET_COMM_WAKEUP_TIMER_TIME_AWAKE:
		if (!FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_set(cmd_data.value))
			return 0;
	case CMD_GET_COMM_WAKEUP_TIMER_TIME_AWAKE:
		cmd_data.value = FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_get();
		break;

	case CMD_SET_MEASUREMENT_WAKEUP_TIMER_INTERVAL:
		if (!FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_set(cmd_data.value))
			return 0;
	case CMD_GET_MEASUREMENT_WAKEUP_TIMER_INTERVAL:
		cmd_data.value = FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_get();
		break;

	case CMD_SET_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE:
		if (!FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set(cmd_data.value))
			return 0;
	case CMD_GET_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE:
		cmd_data.value = FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get();
		break;

	case CMD_SET_HIVE_ID:
		if (!FLASH_HIVE_ID_set(cmd_data.value))
			return 0;
	case CMD_GET_HIVE_ID:
		cmd_data.value = FLASH_HIVE_ID_get();
		break;

	case CMD_SET_NODE_ID:
		if (!FLASH_NODE_ID_set(cmd_data.value))
			return 0;
	case CMD_GET_NODE_ID:
		cmd_data.value = FLASH_NODE_ID_get();
		break;

	default:
		return 0;
	}

	if (!attach_cmd(cmd_data_pkt, &cmd_data))
		return 0;

	cmd_data_pkt->crc16 = crc16_compute((uint8_t*) cmd_data_pkt,
			get_pkt_length(cmd_data_pkt) - CRC_SIZE);

	return 1;
}
