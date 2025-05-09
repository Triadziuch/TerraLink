/*
 * comm.h
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#ifndef INC_COMM_H_
#define INC_COMM_H_

#include "stdbool.h"
#include "string.h"
#include "packet.h"
#include "SX1278.h"
#include "main.h"

#define MAX_RETRIES	3
#define HEADER_SIZE 5
#define CRC_SIZE 2
#define PKT_RX_TIMEOUT 3000
#define PKT_TX_TIMEOUT 3000

void comm_init();
int comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout);
int comm_rx(uint8_t length, uint32_t timeout);

int comm_send(const packet_t *pkt);
int comm_receive(packet_t *pkt);
int comm_check(packet_t *pkt);

int comm_handshake_master(void);

#endif /* INC_COMM_H_ */
