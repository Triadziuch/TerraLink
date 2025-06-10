/*
 * comm.h
 *
 *  Created on: Apr 17, 2025
 *      Author: kurow
 */

#ifndef INC_COMM_H_
#define INC_COMM_H_

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "packet.h"
#include "SX1278.h"
#include "main.h"
#include "time.h"

#define MAX_RETRIES	3
#define PKT_RX_TIMEOUT 3000
#define PKT_TX_TIMEOUT 3000

#define DEBUG_PACKET 0
#define DEBUG_INFO 1

uint32_t GetTime(void);

void comm_init();

int comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout);
int comm_rx(uint8_t length, uint32_t timeout);

int comm_send(const packet_t *pkt);
int comm_receive(packet_t *pkt);

int comm_handshake_slave(const packet_t *received_pkt);
int comm_handle_data(const packet_t *received_pkt);
packet_t* comm_req_data(uint8_t dest_id, DATA_TYPE req_data_type);
int comm_send_ack(const packet_t *received_pkt);

void comm_print_pkt(const packet_t *pkt, const char *text);

#endif /* INC_COMM_H_ */
