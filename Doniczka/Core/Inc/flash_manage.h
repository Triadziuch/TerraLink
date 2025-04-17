#ifndef __flash_manage_H__
#define __flash_manage_H__

// Node UID
#define NODE_UID_ADDR 		0x1FF80050

// Node ID
#define NODE_ID_ADDR 		0x08080000
#define NODE_ID_FLAG_ADDR 	(NODE_ID_ADDR + 1)
#define NODE_ID_VALID_FLAG 	0xA5

#include "stdbool.h"
#include "stm32l0xx_hal.h"
#include "stdio.h"

typedef struct{
	uint32_t UID_0;
	uint32_t UID_1;
	uint32_t UID_2;
} STM32_UID_t;

void FLASH_NODE_UID_get(STM32_UID_t *stm32_uid);

uint8_t FLASH_NODE_ID_get(void);
bool FLASH_NODE_ID_set(uint8_t new_id);
bool FLASH_NODE_ID_is_valid(void);



#endif
