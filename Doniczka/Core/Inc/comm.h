/*
 * comm.h
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#ifndef INC_COMM_H_
#define INC_COMM_H_

#include "stdbool.h"
#include "packet.h"
#include "SX1278.h"
#include "main.h"

void comm_init(SX1278_t *sx);
bool comm_send(const packet_t *pkt);
bool comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout);
bool comm_receive(packet_t *pkt, uint32_t timeout);
bool comm_rx(uint8_t length, uint32_t timeout);

#endif /* INC_COMM_H_ */
