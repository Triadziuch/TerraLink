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
		return 0;

	uint8_t payload_len = rx_buf[4];
	if (payload_len > MAX_PAYLOAD_SIZE)
		return 0;

	uint8_t actual_len = HEADER_SIZE + payload_len + CRC_SIZE;
	uint16_t received_crc;
	memcpy(&received_crc, rx_buf + actual_len - CRC_SIZE, CRC_SIZE);

	uint16_t computed_crc = crc16_compute(rx_buf, actual_len - CRC_SIZE);

	if (computed_crc != received_crc)
		return 0;

	memcpy(pkt, rx_buf, HEADER_SIZE);
	if (payload_len > 0)
		memcpy(pkt->payload, rx_buf + HEADER_SIZE, payload_len);
	memcpy(&pkt->crc16, rx_buf + HEADER_SIZE + payload_len, CRC_SIZE);

	comm_print_pkt(pkt, "Odebrano pakiet");
	return 1;
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
	if (pkt == NULL || cmd_data == NULL || pkt->len < CMD_RECORD_SIZE)
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

//uint8_t set_id(uint8_t current_id, uint8_t new_id){
//	for (int i = 0; i < UIDs_count; ++i)
//		if (UIDs_assigned_ID[i] == current_id){
//			UIDs_assigned_ID[i] = new_id;
//			return 1;
//		}
//	return 0;
//}

uint8_t create_ack_pkt(packet_t *ack_pkt, const packet_t *received_pkt) {
	if (received_pkt == NULL || ack_pkt == NULL)
		return 0;

	ack_pkt->dst_id = received_pkt->src_id;
	ack_pkt->src_id = HIVE_ID;
	ack_pkt->pkt_type = PKT_ACK;
	ack_pkt->seq = received_pkt->seq + 1;
	ack_pkt->len = 0;
	ack_pkt->crc16 = crc16_compute((uint8_t*) ack_pkt,
			get_pkt_length(ack_pkt) - CRC_SIZE);

	return 1;
}

uint8_t create_handshake_response_pkt(packet_t *resp_pkt,
		const packet_t *req_pkt) {
	if (req_pkt->len != 3 * DATA_RECORD_SIZE)
		return 0;

	data_record_t req_data[3];
	for (int i = 0; i < 3; ++i) {
		if (!get_data(req_pkt, i, &req_data[i]))
			return 0;

		if (req_data[i].type != DATA_HANDSHAKE)
			return 0;
	}

	STM32_UID_t uid = { req_data[0].data, req_data[1].data, req_data[2].data };
	int8_t uid_found_pos = find_uid(&uid);
	uint8_t assigned_id = 0;
	if (uid_found_pos >= 0)
		assigned_id = FLASH_NODE_ID_get(uid_found_pos);
	else
		assigned_id = get_new_id();

	data_record_t resp_data;
	resp_data.type = DATA_ID;
	resp_data.time_offset = 0;
	resp_data.data = assigned_id;

	resp_pkt->dst_id = req_pkt->src_id;
	resp_pkt->src_id = HIVE_ID;
	resp_pkt->pkt_type = PKT_ASSIGN_ID;
	resp_pkt->seq = req_pkt->seq + 1;
	resp_pkt->len = 0;
	if (!attach_data(resp_pkt, &resp_data))
		return 0;
	resp_pkt->crc16 = crc16_compute((uint8_t*) resp_pkt,
			get_pkt_length(resp_pkt) - CRC_SIZE);

	return assigned_id;
}

uint8_t create_request_data_pkt(packet_t *req_pkt, uint8_t dest_id,
		DATA_TYPE req_data_type) {
	if (req_pkt == NULL)
		return 0;

	data_record_t req_data_record;
	req_data_record.type = req_data_type;
	req_data_record.time_offset = 0;
	req_data_record.data = 0;

	req_pkt->dst_id = dest_id;
	req_pkt->src_id = HIVE_ID;
	req_pkt->pkt_type = PKT_REQ_DATA;
	req_pkt->seq = next_seq_number();
	req_pkt->len = 0;
	if (!attach_data(req_pkt, &req_data_record))
		return 0;
	req_pkt->crc16 = crc16_compute((uint8_t*) req_pkt,
			get_pkt_length(req_pkt) - CRC_SIZE);

	return 1;
}

uint8_t create_test_conn_pkt(packet_t *test_pkt, uint8_t dest_id) {
	if (test_pkt == NULL)
		return 0;

	test_pkt->dst_id = dest_id;
	test_pkt->src_id = HIVE_ID;
	test_pkt->pkt_type = PKT_TEST_CONN;
	test_pkt->seq = next_seq_number();
	test_pkt->len = 0;
	test_pkt->crc16 = crc16_compute((uint8_t*) test_pkt,
			get_pkt_length(test_pkt) - CRC_SIZE);

	return 1;
}

uint8_t create_cmd_pkt(packet_t *cmd_pkt, uint8_t dest_id, CMD_TYPE cmd,
		uint16_t value) {
	if (cmd_pkt == NULL)
		return 0;

	cmd_pkt->dst_id = dest_id;
	cmd_pkt->src_id = HIVE_ID;
	cmd_pkt->pkt_type = PKT_CMD;
	cmd_pkt->seq = next_seq_number();
	cmd_pkt->len = 0;

	// If set command
	if (cmd % 2 == 0) {
		if (cmd != CMD_SET_COMM_WAKEUP_TIMER_INTERVAL
				&& cmd != CMD_SET_MEASUREMENT_WAKEUP_TIMER_INTERVAL)
			if (value > 255)
				return 0;

		cmd_record_t cmd_record;
		cmd_record.type = cmd;
		cmd_record.value = value;

		if (!attach_cmd(cmd_pkt, &cmd_record))
			return 0;

	} else {
		cmd_record_t cmd_record;
		cmd_record.type = cmd;
		cmd_record.value = 0;

		if (!attach_cmd(cmd_pkt, &cmd_record))
			return 0;
	}

	cmd_pkt->crc16 = crc16_compute((uint8_t*) cmd_pkt,
			get_pkt_length(cmd_pkt) - CRC_SIZE);

	return 1;
}

uint8_t create_start_pkt(packet_t *start_pkt, uint8_t dest_id) {
	if (find_id(dest_id) < 0)
		return 0;

	start_pkt->dst_id = dest_id;
	start_pkt->src_id = HIVE_ID;
	start_pkt->pkt_type = PKT_START;
	start_pkt->seq = next_seq_number();
	start_pkt->len = 0;
	start_pkt->crc16 = crc16_compute((uint8_t*) start_pkt,
			get_pkt_length(start_pkt) - CRC_SIZE);

	return 1;
}
