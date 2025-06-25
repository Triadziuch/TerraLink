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
#include "sensors.h"
#include "time.h"

#define MAX_RETRIES	3
#define PKT_RX_TIMEOUT 3000
#define PKT_TX_TIMEOUT 3000

extern RTC_TimeTypeDef clock_time;
extern RTC_DateTypeDef clock_date;

uint32_t GetTime(void);

void comm_init();
uint8_t comm_tx(uint8_t *txBuf, uint8_t length, uint32_t timeout);
uint8_t comm_rx(uint8_t length, uint32_t timeout);

uint8_t comm_send(const packet_t *pkt);
uint8_t comm_receive(packet_t *pkt);

uint8_t comm_send_moisture(const packet_t *request_pkt);
uint8_t comm_send_lux(const packet_t *request_pkt);
uint8_t comm_send_cmd_data(const packet_t *cmd_packet);
uint8_t comm_send_ack(const packet_t *received_pkt);
uint8_t comm_await_ack(const packet_t *sent_packet);

uint8_t comm_handshake_master(void);
uint8_t comm_handle_req_data(const packet_t *received_pkt);
uint8_t comm_handle_test_conn(const packet_t *received_pkt);
uint8_t comm_handle_cmd(const packet_t *received_pkt);

#endif /* INC_COMM_H_ */
