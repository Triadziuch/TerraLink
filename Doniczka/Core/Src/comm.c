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

int comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout) {
	HAL_NVIC_DisableIRQ(EXTI_LINE);
	int status = SX1278_transmit(&sx1278, txBuf, length, timeout);
	HAL_NVIC_EnableIRQ(EXTI_LINE);
	return status;
}

int comm_rx(uint8_t length, uint32_t timeout) {
	return SX1278_receive(&sx1278, length, timeout);
}

int comm_send(const packet_t *pkt) {
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

int comm_receive(packet_t *pkt) {
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

int comm_handshake_master(void) {
	packet_t req;

	if (!create_handshake_pkt(&req))
		return 0;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&req))
			continue;

		packet_t response;
		if (comm_receive(&response)) {
			if (response.pkt_type
					== PKT_ASSIGN_ID&& response.dst_id == req.src_id && response.len == DATA_RECORD_SIZE) {

				data_record_t data_record;

				if (!get_data(&response, 0, &data_record))
					continue;

				if (data_record.type != DATA_ID)
					continue;

				uint8_t new_id = (uint8_t) data_record.data;

				if (FLASH_NODE_ID_set(new_id)) {
					packet_t ack;
					ack.dst_id = response.src_id;
					ack.src_id = new_id;
					ack.pkt_type = PKT_ACK;
					ack.seq = next_seq_number();
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

int comm_send_moisture(void) {
	sensor_data_raw_t moisture;

	if (!GetSoilMoisturePercentage(&moisture))
		return 0;

	//HAL_Delay(3000);

	data_record_t data_record;
	data_record.type = DATA_SOIL_MOISTURE;
	data_record.time_offset = GetTime() - moisture.time;
	data_record.data = moisture.value;

	packet_t data_pkt;
	create_data_pkt(&data_pkt);
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

int comm_send_lux(void) {
	sensor_data_raw_t light;

	if (!GetLightSensorValue(&light))
		return 0;

	//HAL_Delay(3000);

	data_record_t data_record;
	data_record.type = DATA_LIGHT;
	data_record.time_offset = GetTime() - light.time;
	data_record.data = light.value;

	packet_t data_pkt;
	create_data_pkt(&data_pkt);
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

int comm_handle_req_data(const packet_t *received_pkt) {
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
		return comm_send_moisture();
	} else if (req_data.type == DATA_LIGHT) {
		return comm_send_lux();
	} else if (req_data.type == DATA_TEMP) {
		return 0;
	}

	return 0;
}

int comm_handle_test_conn(const packet_t *received_pkt) {
	if (received_pkt == NULL)
		return 0;

	if (comm_send_ack(received_pkt))
		return 1;

	return 0;
}

int comm_send_ack(const packet_t *received_pkt) {
	packet_t ack_pkt;
	if (!create_ack_pkt(received_pkt, &ack_pkt))
		return 0;

	HAL_Delay(100);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (comm_send(&ack_pkt))
			return 1;


		HAL_Delay(100);
	}

	return 0;
}

int comm_await_ack(const packet_t *sent_packet) {
	packet_t response;
	if (comm_receive(&response)) {
		if (response.pkt_type == PKT_ACK
				&& response.dst_id == sent_packet->src_id
				&& response.src_id == sent_packet->dst_id) {
			return 1;
		}
	}

	return 0;
}
