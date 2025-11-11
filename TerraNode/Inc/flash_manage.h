#ifndef __flash_manage_H__
#define __flash_manage_H__

#include "stm32l0xx_hal.h"
#include "stdio.h"
#include "STMDevice.h"

// Node UID
#define NODE_UID_ADDR 		0x1FF80050

// Node ID
#define NODE_ID_ADDR 		0x08080000 // 1 bajt
#define NODE_ID_FLAG_ADDR 	0x08080001 // 1 bajt
#define LINK_ID_ADDR		0x08080002 // 1 bajt
#define NODE_ID_VALID_FLAG 	0xA5

// Hive ID
#define ADDR_HIVE_ID		0x08080003 // 1 bajt
#define ADDR_HIVE_ID_FLAG	0x08080004 // 1 bajt

#define ADDR_COMM_WAKEUP_TIMER_INTERVAL			0x08080006	// 2 bajty
#define ADDR_COMM_WAKEUP_TIMER_INTERVAL_FLAG	0x08080008	// 1 bajt
#define ADDR_COMM_WAKEUP_TIMER_TIME_AWAKE		0x08080009 	// 1 bajt
#define ADDR_COMM_WAKEUP_TIMER_TIME_AWAKE_FLAG	0x0808000A	// 1 bajt

#define ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL			0x0808000C	// 2 bajty
#define ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_FLAG	0x0808000F	// 1 bajt
#define ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE			0x08080010 	// 1 bajt
#define ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_FLAG		0x08080011	// 1 bajt

extern uint16_t COMM_WAKEUP_TIMER_INTERVAL,
		MEASUREMENT_WAKEUP_TIMER_INTERVAL;
extern uint8_t COMM_WAKEUP_TIMER_TIME_AWAKE,
		MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE;
extern uint8_t HIVE_ID, NODE_ID;

void FLASH_NODE_UID_get(SDeviceUID *stm32_uid);
uint8_t FLASH_NODE_ID_get(void);
uint8_t FLASH_NODE_ID_set(uint8_t new_id);
uint8_t FLASH_NODE_ID_is_valid(void);

uint8_t FLASH_HIVE_ID_get(void);
uint8_t FLASH_HIVE_ID_set(uint8_t hive_id);
uint8_t FLASH_HIVE_ID_is_valid(void);

uint8_t FLASH_COMM_WAKEUP_TIMER_INTERVAL_set(uint16_t interval);
uint16_t FLASH_COMM_WAKEUP_TIMER_INTERVAL_get(void);
uint8_t FLASH_COMM_WAKEUP_TIMER_INTERVAL_is_valid(void);

uint8_t FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_set(uint8_t time);
uint8_t FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_get(void);
uint8_t FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_is_valid(void);

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_set(uint16_t interval);
uint16_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_get(void);
uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_is_valid(void);

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set(uint8_t time);
uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get(void);
uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_is_valid(void);

void node_restart(void);

uint8_t node_config1(void);
#endif
