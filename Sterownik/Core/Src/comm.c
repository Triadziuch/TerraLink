/*
 * comm.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "comm.h"

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

	int valid = comm_check(pkt);
	lora_data_ready = 0;

	return valid;
}

int comm_check(packet_t *pkt) {

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

int comm_handshake_slave(const packet_t *received_pkt) {
	if (received_pkt->pkt_type != PKT_REG_REQ)
		return 0;

	packet_t assign_pkt;
	assign_pkt.dst_id = received_pkt->src_id;
	assign_pkt.src_id = 69;
	assign_pkt.pkt_type = PKT_ASSIGN_ID;
	assign_pkt.seq = next_seq_number();
	assign_pkt.len = 1;
	assign_pkt.payload[0] = 22;
	assign_pkt.crc16 = crc16_compute((uint8_t*)&assign_pkt, get_pkt_length(&assign_pkt) - CRC_SIZE);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&assign_pkt))
			continue;

		packet_t response;
		if (comm_receive(&response)) {

			if (response.pkt_type == PKT_ACK
					&& response.dst_id == assign_pkt.src_id
					&& response.src_id == assign_pkt.payload[0]) {

				return 1;
			}
		}
	}

	return 0;
}

void comm_print_pkt(const packet_t *pkt, const char* text) {
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
