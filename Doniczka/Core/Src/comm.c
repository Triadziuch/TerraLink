/*
 * comm.c
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#include "comm.h"

//SX1278_hw_t sx1278_hw;
static SX1278_t *sx1278;

void comm_init(SX1278_t *sx){
	sx1278 = sx;
}

bool comm_send(const packet_t *pkt){
	return 0;
}

bool comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout){
	return SX1278_transmit(sx1278, txBuf, length, timeout);
}

bool comm_receive(packet_t *pkt, uint32_t timeout){
	return 0;
}

bool comm_rx(uint8_t length, uint32_t timeout){
	return SX1278_receive(sx1278, length, timeout);
}
