/*
 * comm.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "comm.h"

bool init_pins = false;

void comm_init(){
	sx1278_hw.dio0.pin = LORA_DIO0_Pin;
	sx1278_hw.dio0.port = LORA_DIO0_GPIO_Port;
	sx1278_hw.nss.pin = LORA_NSS_Pin;
	sx1278_hw.nss.port = LORA_NSS_GPIO_Port;
	sx1278_hw.reset.pin = LORA_RST_Pin;
	sx1278_hw.reset.port = LORA_RST_GPIO_Port;
	sx1278_hw.spi = &hspi1;

	sx1278.hw = &sx1278_hw;

	SX1278_init(&sx1278,
				433000000,
				SX1278_POWER_17DBM,
				SX1278_LORA_SF_7,
				SX1278_LORA_BW_125KHZ,
				SX1278_LORA_CR_4_5,
				SX1278_LORA_CRC_EN,
				64);
}

bool comm_send(const packet_t *pkt){
	return SX1278_transmit(&sx1278, (uint8_t*)pkt, sizeof(packet_t) - MAX_PAYLOAD_SIZE + pkt->len, 3000);
}

bool comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout){
	return SX1278_transmit(&sx1278, txBuf, length, timeout);
}

bool comm_receive(packet_t *pkt, uint32_t timeout){
	if (!SX1278_receive(&sx1278, sizeof(packet_t), timeout))
		return false;

	uint32_t start_time = HAL_GetTick();
	    while (!lora_data_ready) {
	        if ((HAL_GetTick() - start_time) > timeout) {
	            return false;
	        }
	    }

	uint8_t *rx_buf = lora_buffer;
	uint8_t len = rx_buf[4];

	if (len > MAX_PAYLOAD_SIZE) {
		lora_data_ready = 0;
		return false;
	}

	uint8_t expected_len = sizeof(packet_t) - MAX_PAYLOAD_SIZE + len;
	if (len != expected_len)
		return false;

	uint16_t computed_crc = crc16_compute(rx_buf, expected_len - sizeof(uint16_t));

	uint16_t received_crc;
	memcpy(&received_crc, rx_buf + expected_len - sizeof(uint16_t), sizeof(uint16_t));

	if (computed_crc != received_crc){
		lora_data_ready = 0;
		return false;
	}

	memcpy(pkt, rx_buf, expected_len);

	lora_data_ready = 0;

	return true;
}

bool comm_rx(uint8_t length, uint32_t timeout){
	return SX1278_receive(&sx1278, length, timeout);
}
