/*
 * comm.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "comm.h"

uint32_t GetTime(void) {
	HAL_RTC_GetTime(&hrtc, &clock_time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &clock_date, RTC_FORMAT_BIN);

	struct tm t;
	t.tm_year = 2000 + clock_date.Year - 1900;
	t.tm_mon = clock_date.Month - 1;
	t.tm_mday = clock_date.Date;
	t.tm_hour = clock_time.Hours;
	t.tm_min = clock_time.Minutes;
	t.tm_sec = clock_time.Seconds;
	t.tm_isdst = 0;

	return (uint32_t) mktime(&t);
}

//TODO: Dokończyć
uint8_t execute_cmd(const packet_t *cmd, const packet_t *cmd_data) {
	if (cmd == NULL || cmd_data == NULL)
		return 0;

	cmd_record_t cmd_record;
	if (get_cmd_data(cmd, &cmd_record) == 0)
		return 0;
	CMD_TYPE type = cmd_record.type;

	switch (type) {
	case CMD_SET_COMM_WAKEUP_TIMER_INTERVAL:

	}
}

void comm_init() {
	sx1278_hw.dio0.pin = LORA_DIO0_Pin;
	sx1278_hw.dio0.port = LORA_DIO0_GPIO_Port;
	sx1278_hw.nss.pin = LORA_NSS_Pin;
	sx1278_hw.nss.port = LORA_NSS_GPIO_Port;
	sx1278_hw.reset.pin = LORA_RST_Pin;
	sx1278_hw.reset.port = LORA_RST_GPIO_Port;
	sx1278_hw.spi = &hspi1;

	sx1278.hw = &sx1278_hw;

	SX1278_init(&sx1278, 433000000,
	SX1278_POWER_17DBM,
	SX1278_LORA_SF_7,
	SX1278_LORA_BW_125KHZ,
	SX1278_LORA_CR_4_5,
	SX1278_LORA_CRC_EN, 64);
}

uint8_t comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout) {
	HAL_NVIC_DisableIRQ(EXTI_LINE);
	int status = SX1278_transmit(&sx1278, txBuf, length, timeout);
	HAL_NVIC_EnableIRQ(EXTI_LINE);
	return status;
}

uint8_t comm_rx(uint8_t length, uint32_t timeout) {
	return SX1278_receive(&sx1278, length, timeout);
}

uint8_t comm_send(const packet_t *pkt) {
	HAL_NVIC_DisableIRQ(EXTI_LINE);

	uint16_t total_len = get_pkt_length(pkt);
	uint8_t buffer[sizeof(packet_t)];

	memcpy(buffer, pkt, total_len - CRC_SIZE);

	buffer[total_len - 2] = pkt->crc16 & 0xFF;
	buffer[total_len - 1] = pkt->crc16 >> 8;

	bool status = SX1278_transmit(&sx1278, buffer, total_len, PKT_TX_TIMEOUT);

	comm_print_pkt(pkt, "Wyslano pakiet");
	HAL_NVIC_EnableIRQ(EXTI_LINE);

	return status;
}

uint8_t comm_receive(packet_t *pkt) {
	lora_data_ready = 0;

	if (!SX1278_receive(&sx1278, sizeof(packet_t), PKT_RX_TIMEOUT))
		return 0;

	uint32_t start_time = HAL_GetTick();
	while (!lora_data_ready)
		if ((HAL_GetTick() - start_time) > PKT_RX_TIMEOUT)
			return 0;

	int valid = verify_pkt(pkt);
	lora_data_ready = 0;

	HAL_Delay(20);

	return valid;
}

uint8_t comm_handshake_slave(const packet_t *received_pkt) {
	if (received_pkt == NULL)
		return 0;

	if (received_pkt->pkt_type != PKT_REG_REQ)
		return 0;

	if (DEBUG_INFO)
		printf("[ID: %d] Received PKT_REG_REQ from device ID = %d\n", HIVE_ID,
				received_pkt->src_id);

	packet_t assign_pkt;
	uint8_t assigned_id = create_handshake_response_pkt(&assign_pkt,
			received_pkt);
	if (assigned_id == 0)
		return 0;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (DEBUG_INFO)
			printf(
					"[ID: %d] Sending PKT_ASSIGN_ID = %d to device ID = %d\t\t[attempt = %d]\n",
					HIVE_ID, assigned_id, assign_pkt.dst_id, attempt);

		if (!comm_send(&assign_pkt))
			continue;

		packet_t response;
		if (comm_receive(&response)) {
			if (response.pkt_type == PKT_ACK
					&& response.dst_id == assign_pkt.src_id
					&& response.src_id == assigned_id
					&& response.seq == assign_pkt.seq + 1) {

				data_record_t req_data[3];
				for (int i = 0; i < 3; ++i)
					get_data(received_pkt, i, &req_data[i]);
				STM32_UID_t uid = { req_data[0].data, req_data[1].data,
						req_data[2].data };
				if (FLASH_NODE_UID_ID_add(&uid, assigned_id) == 0)
					return 0;

				// Node config
				if (configure_new_node(assigned_id) == 0)
					return 0;

				if (DEBUG_INFO)
					printf(
							"[ID: %d] Received handshake PKT_ACK from device ID = %d\n",
							HIVE_ID, response.src_id);

				return 1;
			}
		}
	}

	return 0;
}

uint8_t configure_new_node(uint8_t node_id) {

	// Comm wakeup timer interval
	if (!configure_field(node_id, CMD_GET_COMM_WAKEUP_TIMER_INTERVAL,
			CMD_SET_COMM_WAKEUP_TIMER_INTERVAL,
			DEFAULT_COMM_WAKEUP_TIMER_INTERVAL,
			(uint8_t (*)(uint8_t, uint32_t)) FLASH_NODE_COMM_WAKEUP_TIMER_INTERVAL_set))
		return 0;

	// Comm wakeup timer time awake
	if (!configure_field(node_id, CMD_GET_COMM_WAKEUP_TIMER_TIME_AWAKE,
			CMD_SET_COMM_WAKEUP_TIME_AWAKE,
			DEFAULT_COMM_WAKEUP_TIMER_TIME_AWAKE,
			(uint8_t (*)(uint8_t, uint32_t)) FLASH_NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_set))
		return 0;

	// Measurement wakeup timer interval
	if (!configure_field(node_id, CMD_GET_MEASUREMENT_WAKEUP_TIMER_INTERVAL,
			CMD_SET_MEASUREMENT_WAKEUP_TIMER_INTERVAL,
			DEFAULT_MEASUREMENT_WAKEUP_TIMER_INTERVAL,
			(uint8_t (*)(uint8_t, uint32_t)) FLASH_NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_set))
		return 0;

	// Measurement wakeup timer time awake
	if (!configure_field(node_id, CMD_GET_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE,
			CMD_SET_MEASUREMENT_WAKEUP_TIME_AWAKE,
			DEFAULT_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE,
			(uint8_t (*)(uint8_t, uint32_t)) FLASH_NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set))
		return 0;

	return 1;
}

uint8_t configure_field(uint8_t node_id, uint8_t get_cmd, uint8_t set_cmd,
		uint16_t default_value, uint8_t (*flash_set)(uint8_t, uint16_t)) {

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		cmd_record_t *record = comm_send_cmd(node_id, get_cmd, 0);

		if (record == NULL)
			continue;

		uint16_t value = record->value;
		if (value == 0) {
			free(record);

			record = comm_send_cmd(node_id, set_cmd, default_value);

			if (record == NULL)
				continue;

			if (record->value != default_value) {
				free(record);
				continue;
			}

			value = record->value;
		}

		if (flash_set(node_id, value)) {
			free(record);
			return 1;
		}

		free(record);
	}

	return 0;
}

uint8_t comm_handle_data(const packet_t *received_pkt) {
	if (received_pkt->pkt_type != PKT_DATA)
		return 0;

	if (DEBUG_INFO)
		printf("[ID: %d] Received PKT_DATA from device ID = %d\n",
		HIVE_ID, received_pkt->src_id);

	if (received_pkt->len < DATA_RECORD_SIZE)
		return 0;

	int data_records_count = received_pkt->len / DATA_RECORD_SIZE;
	data_record_t *data_records = malloc(data_records_count * DATA_RECORD_SIZE);

	if (data_records == NULL)
		return 0;

	time_t time_now = (time_t) GetTime();
	for (int record_id = 0; record_id < data_records_count; ++record_id) {
		if (!get_data(received_pkt, record_id, &data_records[record_id]))
			return 0;

		time_t measurement_time = time_now
				- data_records[record_id].time_offset;
		struct tm *t = localtime(&measurement_time);

		if (DEBUG_INFO == 0)
			continue;

		if (data_records[record_id].type == DATA_SOIL_MOISTURE) {
			printf(
					"[ID: %d] [%02d:%02d:%02d] Wilgotnosc: %u.%u o czasie %02d:%02d:%02d\t\t\t[record_id = %d]\n",
					HIVE_ID, clock_time.Hours, clock_time.Minutes,
					clock_time.Seconds, data_records[record_id].data / 10,
					data_records[record_id].data % 10, t->tm_hour, t->tm_min,
					t->tm_sec, record_id);
		} else if (data_records[record_id].type == DATA_LIGHT) {
			printf(
					"[ID: %d] [%02d:%02d:%02d] Natezenie swiatla: %u o czasie %02d:%02d:%02d\t\t[record_id = %d]\n",
					HIVE_ID, clock_time.Hours, clock_time.Minutes,
					clock_time.Seconds, data_records[record_id].data,
					t->tm_hour, t->tm_min, t->tm_sec, record_id);

		} else
			return 0;

	}

	return comm_send_ack(received_pkt); //TODO: Check if received data comes from request - in that case don't send ACK
}

// Returns received packet with data. Returns NULL if didn't get response to request.
packet_t* comm_req_data(uint8_t dest_id, DATA_TYPE req_data_type) {
	if (find_id(dest_id) < 0)
		return NULL;

	packet_t req_data_pkt;
	if (create_request_data_pkt(&req_data_pkt, dest_id, req_data_type) == 0)
		return NULL;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&req_data_pkt))
			continue;

		if (DEBUG_INFO)
			printf(
					"[ID: %d] Sending PKT_REQ_DATA = %d to device ID = %d\t\t[attempt = %d]\n",
					HIVE_ID, req_data_type, req_data_pkt.dst_id, attempt);

		packet_t *response = malloc(sizeof(packet_t));
		if (comm_receive(response)) {
			if (response->pkt_type == PKT_DATA
					&& response->dst_id == req_data_pkt.src_id
					&& response->src_id == req_data_pkt.dst_id
					&& response->seq == req_data_pkt.seq + 1) {

				if (DEBUG_INFO)
					printf(
							"[ID: %d] Received requested data PKT_DATA from device ID = %d\t\t[attempt = %d]\n",
							HIVE_ID, response->src_id, attempt);

				if (!comm_send_ack(response))
					return NULL;

				return response;
			}

		}
	}
	return NULL;
}

// Returns received packed with data, either from SET or GET command. Returns NULL if didn't get response to request.
cmd_record_t* comm_send_cmd(uint8_t dest_id, CMD_TYPE cmd_type, uint16_t value) {
	if (find_id(dest_id) < 0)
		return NULL;

	packet_t cmd_pkt;
	if (create_cmd_pkt(&cmd_pkt, dest_id, cmd_type, value) == 0)
		return NULL;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&cmd_pkt))
			continue;

		if (DEBUG_INFO)
			printf(
					"[ID: %d] Sending CMD_PKT = %d to device ID = %d\t\t[attempt = %d]\n",
					HIVE_ID, cmd_type, cmd_pkt.dst_id, attempt);

		packet_t response;
		if (comm_receive(&response)) {
			if (response.pkt_type == PKT_CMD_DATA
					&& response.dst_id == cmd_pkt.src_id
					&& response.src_id == cmd_pkt.dst_id
					&& response.seq == cmd_pkt.seq + 1) {

				cmd_record_t *cmd_record = malloc(sizeof(cmd_record_t));
				if (!get_cmd_data(&response, cmd_record)
						&& cmd_record->type != cmd_type)
					continue;

				if (DEBUG_INFO)
					printf(
							"[ID: %d] Received CMD confirmation PKT_CMD_DATA = %d, value = %d from device ID = %d\t\t[attempt = %d]\n",
							HIVE_ID, cmd_record->type, cmd_record->value,
							response.src_id, attempt);

				if (!comm_send_ack(&response))
					return NULL;

				return cmd_record;
			}

		}
	}
	return NULL;
}

uint8_t comm_send_ack(const packet_t *received_pkt) {
	packet_t ack_pkt;
	if (!create_ack_pkt(&ack_pkt, received_pkt))
		return 0;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (comm_send(&ack_pkt)) {
			if (DEBUG_INFO)
				printf(
						"[ID: %d] Sent PKT_ACK to PKT_DATA from device ID = %d\t\t\t[attempt = %d]\n",
						HIVE_ID, received_pkt->src_id, attempt);

			return 1;
		}

		HAL_Delay(100);
	}

	return 0;
}

uint8_t comm_test_conn(uint8_t link_id) {
	if (find_id(link_id) < 0)
		return 0;

	packet_t test_pkt;
	if (!create_test_conn_pkt(&test_pkt, link_id))
		return 0;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (comm_send(&test_pkt)) {
			if (DEBUG_INFO)
				printf(
						"[ID: %d] Sent PKT_TEST_CONN to device ID = %d\t\t\t[attempt = %d]\n",
						HIVE_ID, test_pkt.dst_id, attempt);

			if (comm_await_ack(&test_pkt))
				return 1;

			HAL_Delay(100);
		}
	}
	return 0;
}

uint8_t comm_await_ack(const packet_t *sent_packet) {
	packet_t response;
	if (comm_receive(&response)) {
		if (response.pkt_type == PKT_ACK
				&& response.dst_id == sent_packet->src_id
				&& response.src_id == sent_packet->dst_id
				&& response.seq == sent_packet->seq + 1) {

			if (DEBUG_INFO)
				printf("[ID: %d] Received PKT_ACK from device ID = %d\n",
				HIVE_ID, response.src_id);
			return 1;
		}
	}

	return 0;
}

void comm_print_pkt(const packet_t *pkt, const char *text) {
	if (DEBUG_PACKET == 0)
		return;

	printf("= = = = = %s = = = = =\n", text);
	printf("[dst_id] = %u\n", pkt->dst_id);
	printf("[src_id] = %u\n", pkt->src_id);
	printf("[pkt_type] = %u\n", pkt->pkt_type);
	printf("[seq] = %u\n", pkt->seq);
	printf("[len] = %u\n", pkt->len);

	printf("[payload]  = ");
	for (uint8_t i = 0; i < pkt->len; ++i) {
		uint8_t c = pkt->payload[i];
		if (c >= 32 && c <= 126)
			printf("%c ", c);
		else
			printf("\\x%02X ", c);
	}
	printf("\n");

	printf("[crc16] = 0x%04X\n\n", pkt->crc16);
}
