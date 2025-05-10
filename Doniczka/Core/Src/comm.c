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

int comm_handshake_master(void) {
	packet_t req;
	req.dst_id = 255; // broadcast
	req.src_id = 0; //FLASH_NODE_ID_get();
	req.pkt_type = PKT_REG_REQ;
	req.seq = next_seq_number();
	req.len = 0;
	req.crc16 = crc16_compute((uint8_t*) &req, get_pkt_length(&req) - CRC_SIZE);

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

				if (data_record.data_type != DATA_ID)
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
					return 1;
				}
			}
		}
	}

	return 0;
}
