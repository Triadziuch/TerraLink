/*
 * flash_manage.h
 *
 *  Created on: Jun 23, 2025
 *      Author: kurow
 */

#ifndef INC_FLASH_MANAGE_H_
#define INC_FLASH_MANAGE_H_

#include "stm32l4xx_hal.h"
#include "stdio.h"
#include "string.h"

#define FLASH_PAGE_ADDR    0x08080000U
#define FLASH_PAGE_SIZE    2048U
#define FLASH_BANK         FLASH_BANK_2

// Node ID
#define NODE_UID_ADDR 		0x08080000 // po 12 bajtów, po każdym 1 bajt na NODE_ID_VALID_FLAG i 1 bajt przerwy dla wyrównania, łącznie miejsca na 8 14-bajtowych komórek
#define NODE_ID_ADDR		0x08080070 // po 2 bajty, łącznie miejsca na 8 komórek
#define NODE_ID_COUNT		0x08080080 // 1 bajt
#define NODE_ID_COUNT_FLAG	0x08080081 // 1 bajt
#define RESERVED_MEMORY_SIZE ((NODE_ID_COUNT_FLAG - NODE_UID_ADDR) + 1)
#define VALID_FLAG 	0xA5
#define NODE_UID_SIZE 14
#define NODE_ID_SIZE 2
#define NODE_MAX_COUNT 8
#define UID_ID_OFFSET (NODE_ID_ADDR - NODE_UID_ADDR) //TODO: Upewnić się

//TODO: Add HIVE ID handling

typedef struct {
	uint32_t UID_0;
	uint32_t UID_1;
	uint32_t UID_2;
} STM32_UID_t;

uint8_t check_UID_ID_flag(uint8_t n);
uint8_t get_new_id(void);
int8_t find_id(uint8_t id);
int8_t find_uid(const STM32_UID_t *uid);
HAL_StatusTypeDef flash_write(uint32_t address, const uint8_t *data, uint16_t length);

uint8_t FLASH_NODE_UID_get(STM32_UID_t *stm32_uid, uint8_t node_id);
uint8_t FLASH_NODE_ID_by_uid_get(STM32_UID_t *stm32_uid);
uint8_t FLASH_NODE_ID_get(uint8_t n);
uint8_t FLASH_NODE_UID_ID_add(STM32_UID_t *stm32_uid, uint8_t assigned_id);
uint8_t FLASH_NODE_ID_update(uint8_t old_id, uint8_t new_id);
uint8_t FLASH_NODE_ID_remove(uint8_t id);
uint8_t FLASH_NODE_count_get(void);

HAL_StatusTypeDef link_config(void);

#endif /* INC_FLASH_MANAGE_H_ */
