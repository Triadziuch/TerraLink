/*
 * comm.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "comm.h"

bool init_pins = false;

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

bool comm_send(const packet_t *pkt) {
	HAL_NVIC_DisableIRQ(EXTI_LINE);

	uint16_t total_len = get_pkt_length(pkt);
	uint8_t buffer[sizeof(packet_t)];

	memcpy(buffer, pkt, total_len - CRC_SIZE);

	buffer[total_len - 2] = pkt->crc16 & 0xFF;
	buffer[total_len - 1] = pkt->crc16 >> 8;

	bool status = SX1278_transmit(&sx1278, buffer, total_len, 3000);
	HAL_NVIC_EnableIRQ(EXTI_LINE);

	return status;
}

bool comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout) {
	return SX1278_transmit(&sx1278, txBuf, length, timeout);
}

bool comm_receive(packet_t *pkt, uint32_t timeout) {
	lora_data_ready = 0;

	if (!SX1278_receive(&sx1278, sizeof(packet_t), timeout))
		return false;

	uint32_t start_time = HAL_GetTick();
	while (!lora_data_ready) {
		if ((HAL_GetTick() - start_time) > timeout) {
			return false;
		}
	}

	uint8_t *rx_buf = lora_buffer;
	if (rx_buf == NULL) {
		lora_data_ready = 0;
		return false;
	}

	uint8_t payload_len = rx_buf[4];
	if (payload_len > MAX_PAYLOAD_SIZE) {
		lora_data_ready = 0;
		return false;
	}

	uint8_t actual_len = HEADER_SIZE + payload_len + CRC_SIZE;
	uint16_t received_crc;
	memcpy(&received_crc, rx_buf + actual_len - CRC_SIZE, CRC_SIZE);

	uint16_t computed_crc = crc16_compute(rx_buf, actual_len - CRC_SIZE);

	if (computed_crc != received_crc) {
		lora_data_ready = 0;
		return false;
	}

	memcpy(pkt, rx_buf, HEADER_SIZE);
	if (payload_len > 0)
		memcpy(pkt->payload, rx_buf + HEADER_SIZE, payload_len);
	memcpy(&pkt->crc16, rx_buf + HEADER_SIZE + payload_len, CRC_SIZE);

	lora_data_ready = 0;
	return true;
}

bool comm_rx(uint8_t length, uint32_t timeout) {
	return SX1278_receive(&sx1278, length, timeout);
}

bool handshake_master(void) {
	packet_t req;
	req.dst_id = 255; // broadcast
	req.src_id = 0; //FLASH_NODE_ID_get();
	req.pkt_type = PKT_REG_REQ;
	req.seq = next_seq_number();
	req.len = 0;
	req.crc16 = crc16_compute((uint8_t*)&req, get_pkt_length(&req) - CRC_SIZE);

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		HAL_Delay(100);

		if (!comm_send(&req))
			continue;

		packet_t response;
		if (comm_receive(&response, 1000)) {
			if (response.pkt_type == PKT_ASSIGN_ID
					&& response.dst_id == req.src_id) {

				uint8_t new_id = response.payload[0];

				if (FLASH_NODE_ID_set(new_id)) {
					packet_t ack;
					ack.dst_id = response.src_id;
					ack.src_id = new_id;
					ack.pkt_type = PKT_ACK;
					ack.seq = next_seq_number();
					ack.len = 0;
					ack.crc16 = crc16_compute((uint8_t*)&ack, get_pkt_length(&ack) - CRC_SIZE);

					if (!comm_send(&ack))
						continue;
					return true;
				}
			}
		}
	}

	return false;
}

bool handshake_slave(const packet_t *received_pkt) {
	if (received_pkt->pkt_type != PKT_REG_REQ)
		return false;

	print_pkt(received_pkt);

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

		print_pkt(&assign_pkt);

		packet_t response;
		if (comm_receive(&response, 1000)) {

			print_pkt(&response);

			if (response.pkt_type == PKT_ACK
					&& response.dst_id == assign_pkt.src_id
					&& response.src_id == assign_pkt.payload[0]) {

				return true;
			}
		}
	}

	return false;
}

void print_pkt(const packet_t *pkt) {
	printf("= = = = = Zawartosc pakietu = = = = =\n");
	printf("[dst_id] = %u\n", pkt->dst_id);
	printf("[src_id] = %u\n", pkt->src_id);
	printf("[pkt_type] = %u\n", pkt->pkt_type);
	printf("[seq] = %u\n", pkt->seq);
	printf("[len] = %u\n", pkt->len);

	char payload_str[MAX_PAYLOAD_SIZE + 1] = { 0 };
	memcpy(payload_str, pkt->payload, pkt->len);
	printf("[payload] = \"%s\"\n", payload_str);
	printf("[crc16] = 0x%04X\n\n", pkt->crc16);
}
