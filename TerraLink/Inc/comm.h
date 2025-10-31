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
uint8_t execute_cmd(const packet_t* cmd, const packet_t* cmd_data);

void comm_init();
uint8_t comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout);
uint8_t comm_rx(uint8_t length, uint32_t timeout);

uint8_t comm_send(const packet_t *pkt);
uint8_t comm_receive(packet_t *pkt);

packet_t* comm_req_data(uint8_t dest_id, DATA_TYPE req_data_type);
cmd_record_t* comm_send_cmd(uint8_t dest_id, CMD_TYPE cmd_type, uint16_t value);
uint8_t comm_send_ack(const packet_t *received_pkt);
uint8_t comm_await_ack(const packet_t *sent_packet);

uint8_t comm_handshake_slave(const packet_t *received_pkt);
uint8_t configure_new_node(uint8_t node_id);
uint8_t configure_field(uint8_t node_id, uint8_t get_cmd, uint8_t set_cmd, uint32_t default_value, uint8_t (*flash_set)(uint8_t, uint32_t));
uint8_t comm_handle_data(const packet_t *received_pkt);
uint8_t comm_handle_cmd_data(const packet_t *received_pkt);

void comm_print_pkt(const packet_t *pkt, const char *text);

#endif /* INC_COMM_H_ */
