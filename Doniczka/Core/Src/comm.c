/*
 * comm.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "comm.h"

int16_t next_comm_wakeup_in, next_measurement_wakeup_in;

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

uint8_t comm_send_moisture(const packet_t *request_pkt) {
	sensor_data_raw_t moisture;

	if (!GetSoilMoisturePercentage(&moisture))
		return 0;

	data_record_t data_record;
	data_record.type = DATA_SOIL_MOISTURE;
	data_record.time_offset = GetTime() - moisture.time;
	data_record.data = moisture.value;

	packet_t data_pkt;
	create_data_pkt(&data_pkt, request_pkt);
	attach_data(&data_pkt, &data_record);
	data_pkt.crc16 = crc16_compute((uint8_t*) &data_pkt,
			get_pkt_length(&data_pkt) - CRC_SIZE);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (!comm_send(&data_pkt))
			continue;

		if (comm_await_ack(&data_pkt))
			return 1;

		HAL_Delay(100);
	}

	return 0;
}

uint8_t comm_send_lux(const packet_t *request_pkt) {
	sensor_data_raw_t light;

	if (!GetLightSensorValue(&light))
		return 0;

	data_record_t data_record;
	data_record.type = DATA_LIGHT;
	data_record.time_offset = GetTime() - light.time;
	data_record.data = light.value;

	packet_t data_pkt;
	create_data_pkt(&data_pkt, request_pkt);
	attach_data(&data_pkt, &data_record);
	data_pkt.crc16 = crc16_compute((uint8_t*) &data_pkt,
			get_pkt_length(&data_pkt) - CRC_SIZE);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (!comm_send(&data_pkt))
			continue;

		if (comm_await_ack(&data_pkt))
			return 1;

		HAL_Delay(100);
	}

	return 0;
}

//TODO: Refactorize redundant code
uint8_t comm_send_cmd_data(const packet_t *cmd_packet) {
	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (!comm_send(cmd_packet))
			continue;

		if (comm_await_ack(cmd_packet))
			return 1;

		HAL_Delay(100);
	}

	return 0;
}

wakeup_t* comm_send_next_wakeup(void) {
	static WAKEUP_TYPE next_wakeup = MEASUREMENT;
	uint16_t sleep_time = 0;

	// Next wakeup is measurement
	if (next_measurement_wakeup_in <= next_comm_wakeup_in) {
		next_wakeup = MEASUREMENT;

		if (next_comm_wakeup_in <= MEASUREMENT_WAKEUP_TIMER_INTERVAL)
			sleep_time = next_comm_wakeup_in;
		else
			sleep_time = MEASUREMENT_WAKEUP_TIMER_INTERVAL;

		next_comm_wakeup_in -= sleep_time;
		next_measurement_wakeup_in = MEASUREMENT_WAKEUP_TIMER_INTERVAL;
	} // is communication
	else {
		next_wakeup = COMMUNICATION;

		if (next_measurement_wakeup_in <= COMM_WAKEUP_TIMER_INTERVAL)
			sleep_time = next_measurement_wakeup_in;
		else
			sleep_time = COMM_WAKEUP_TIMER_INTERVAL;

		next_comm_wakeup_in = COMM_WAKEUP_TIMER_INTERVAL;
		next_measurement_wakeup_in -= sleep_time;
	}

	cmd_record_t wakeup_record;
	wakeup_record.type = CMD_INFO_NEXT_WAKEUP;
	wakeup_record.value = sleep_time;

	packet_t *pkt = malloc(sizeof(packet_t));
	pkt->dst_id = HIVE_ID;
	pkt->src_id = NODE_ID;
	pkt->pkt_type = PKT_CMD_DATA;
	pkt->seq = next_seq_number();
	pkt->len = 0;
	if (!attach_cmd(pkt, &wakeup_record))
		return 0;
	pkt->crc16 = crc16_compute((uint8_t*) pkt, get_pkt_length(pkt) - CRC_SIZE);

	if (!comm_send_cmd_data(pkt)) {
		free(pkt);
		return NULL;
	}
	free(pkt);

	wakeup_t *wakeup = malloc(sizeof(wakeup_t));
	wakeup->wakeup_type = next_wakeup;
	wakeup->time = sleep_time;
	return wakeup;
}

uint8_t comm_send_ack(const packet_t *received_pkt) {
	packet_t ack_pkt;
	if (!create_ack_pkt(&ack_pkt, received_pkt))
		return 0;

	HAL_Delay(100);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (comm_send(&ack_pkt))
			return 1;

		HAL_Delay(100);
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
			return 1;
		}
	}

	return 0;
}

uint8_t comm_await_start(void) {
	packet_t packet;
	if (comm_receive(&packet) && packet.dst_id == NODE_ID) {
		if (packet.pkt_type == PKT_START) {
			next_comm_wakeup_in = COMM_WAKEUP_TIMER_INTERVAL;
			next_measurement_wakeup_in = MEASUREMENT_WAKEUP_TIMER_INTERVAL;
			return 1;
		}

		else if (packet.pkt_type == PKT_CMD)
			comm_handle_cmd(&packet);
	}
	return 0;
}

uint8_t comm_handshake_master(void) {
	packet_t req;

	if (!create_handshake_pkt(&req))
		return 0;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&req))
			continue;

		packet_t response;
		if (comm_receive(&response)) {
			if (response.pkt_type == PKT_ASSIGN_ID
					&& response.dst_id == req.src_id
					&& response.len == DATA_RECORD_SIZE
					&& response.seq == req.seq + 1) {

				data_record_t data_record;

				if (!get_data(&response, 0, &data_record))
					continue;

				if (data_record.type != DATA_ID)
					continue;

				uint8_t new_id = (uint8_t) data_record.data;

				if (FLASH_NODE_ID_set(new_id)
						&& FLASH_HIVE_ID_set(response.src_id)) {
					packet_t ack;
					ack.dst_id = response.src_id;
					ack.src_id = NODE_ID;
					ack.pkt_type = PKT_ACK;
					ack.seq = response.seq + 1;
					ack.len = 0;
					ack.crc16 = crc16_compute((uint8_t*) &ack,
							get_pkt_length(&ack) - CRC_SIZE);

					if (!comm_send(&ack))
						continue;

				}
				return 1;

			}
		}
	}

	return 0;
}

uint8_t comm_handle_req_data(const packet_t *received_pkt) {
	if (received_pkt == NULL)
		return 0;

	if (received_pkt->pkt_type != PKT_REQ_DATA)
		return 0;

	data_record_t req_data;
	if (!get_data(received_pkt, 0, &req_data))
		return 0;

	if (req_data.type == DATA_ID) {
		return 0;
	} else if (req_data.type == DATA_SOIL_MOISTURE) {
		return comm_send_moisture(received_pkt);
	} else if (req_data.type == DATA_LIGHT) {
		return comm_send_lux(received_pkt);
	} else if (req_data.type == DATA_TEMP) {
		return 0;
	}

	return 0;
}

uint8_t comm_handle_test_conn(const packet_t *received_pkt) {
	if (received_pkt == NULL)
		return 0;

	if (comm_send_ack(received_pkt))
		return 1;

	return 0;
}

uint8_t comm_handle_cmd(const packet_t *received_pkt) {
	if (received_pkt == NULL)
		return 0;

	if (received_pkt->pkt_type != PKT_CMD)
		return 0;

	packet_t cmd_response;
	if (create_cmd_data_resp_pkt(&cmd_response, received_pkt) == 0)
		return 0;

	return comm_send_cmd_data(&cmd_response);
}
