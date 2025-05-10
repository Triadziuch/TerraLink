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

	comm_print_pkt(pkt, "Wyslano pakiet");
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

	return valid;
}

int comm_handshake_slave(const packet_t *received_pkt) {
	if (received_pkt->pkt_type != PKT_REG_REQ)
		return 0;

	data_record_t data;
	data.type = DATA_ID;
	data.time_offset = 0;
	data.data = 22;

	packet_t assign_pkt;
	assign_pkt.dst_id = received_pkt->src_id;
	assign_pkt.src_id = 69;
	assign_pkt.pkt_type = PKT_ASSIGN_ID;
	assign_pkt.seq = next_seq_number();
	assign_pkt.len = 0;
	attach_data(&assign_pkt, &data);
	assign_pkt.crc16 = crc16_compute((uint8_t*) &assign_pkt,
			get_pkt_length(&assign_pkt) - CRC_SIZE);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&assign_pkt))
			continue;

		packet_t response;
		if (comm_receive(&response)) {
			if (response.pkt_type == PKT_ACK
					&& response.dst_id == assign_pkt.src_id
					&& response.src_id == data.data)
				return 1;

		}
	}

	return 0;
}

int comm_handle_data(const packet_t *received_pkt) {
	if (received_pkt->pkt_type != PKT_DATA)
		return 0;

	if (received_pkt->len < DATA_RECORD_SIZE)
		return 0;

	int data_records_count = received_pkt->len / DATA_RECORD_SIZE;
	data_record_t *data_records = malloc(data_records_count * DATA_RECORD_SIZE);

	if (data_records == NULL)
		return 0;

	time_t time_now = (time_t)GetTime();
	for (int record_id = 0; record_id < data_records_count; ++record_id) {
		if (!get_data(received_pkt, record_id, &data_records[record_id]))
			return 0;

		if (data_records[record_id].type != DATA_TEMP)
			return 0;

		time_t measurement_time = time_now
				- data_records[record_id].time_offset;
		struct tm *t = localtime(&measurement_time);

		printf("[%02d:%02d:%02d] Wilgotnosc: %u.%u o czasie %02d:%02d:%02d\n",
				clock_time.Hours, clock_time.Minutes, clock_time.Seconds,
				data_records[record_id].data / 10,
				data_records[record_id].data % 10, t->tm_hour, t->tm_min,
				t->tm_sec);
	}

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		packet_t ack;
		ack.dst_id = received_pkt->src_id;
		ack.src_id = 69;
		ack.pkt_type = PKT_ACK;
		ack.seq = next_seq_number();
		ack.len = 0;
		ack.crc16 = crc16_compute((uint8_t*) &ack,
				get_pkt_length(&ack) - CRC_SIZE);

		if (comm_send(&ack))
			return 1;

		HAL_Delay(100);
	}

	return 0;
}

void comm_print_pkt(const packet_t *pkt, const char *text) {
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
