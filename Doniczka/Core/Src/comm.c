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
	return 0;
}

bool comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout){
	return SX1278_transmit(&sx1278, txBuf, length, timeout);
}

bool comm_receive(packet_t *pkt, uint32_t timeout){
	return 0;
}

bool comm_rx(uint8_t length, uint32_t timeout){
	return SX1278_receive(&sx1278, length, timeout);
}
