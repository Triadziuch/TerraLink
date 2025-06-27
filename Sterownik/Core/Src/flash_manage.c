/*
 * flash_manage.c
 *
 *  Created on: Jun 23, 2025
 *      Author: kurow
 */
#include "flash_manage.h"

//int UIDs_count = 0;
//STM32_UID_t UIDs[8] = { 0 };
//uint8_t UIDs_assigned_ID[8] = { 0 };

uint8_t check_UID_ID_flag(uint8_t n) {
	if (*((uint8_t*) (NODE_ID_ADDR + n * NODE_DATA_SIZE + 1)) == VALID_FLAG
			&& *((uint8_t*) (NODE_UID_ADDR + n * NODE_DATA_ADDR + 12))
					== VALID_FLAG)
		return 1;
	return 0;
}

uint8_t get_new_id(void) {
	uint8_t new_id = (*(uint8_t*) NODE_COUNT_ADDR);
	while (find_id(++new_id) >= 0) {
	}
	return new_id;
}

int8_t find_id(uint8_t id) {
	for (uint8_t i = 0; i < *(uint8_t*) NODE_COUNT_ADDR; ++i)
		if (*((uint8_t*) (NODE_ID_ADDR + i * NODE_DATA_ADDR)) == id
				&& check_UID_ID_flag(i))
			return i;
	return -1;
}

int8_t find_uid(const STM32_UID_t *uid) {
	for (uint8_t i = 0; i < *(uint8_t*) NODE_COUNT_ADDR; ++i)
		if (memcmp((void*) NODE_UID_ADDR + i * NODE_DATA_ADDR, uid,
				sizeof(STM32_UID_t)) == 0 && check_UID_ID_flag(i))
			return i;
	return -1;
}

HAL_StatusTypeDef flash_write(uint32_t address, const uint8_t *data,
		uint16_t length) {
	if (address < FLASH_PAGE_ADDR
			|| (address + length) > (FLASH_PAGE_ADDR + FLASH_PAGE_SIZE))
		return HAL_ERROR;

	uint8_t pageBuffer[FLASH_PAGE_SIZE];
	memcpy(pageBuffer, (uint8_t*) FLASH_PAGE_ADDR, FLASH_PAGE_SIZE);
	memcpy(&pageBuffer[address - FLASH_PAGE_ADDR], data, length);

	HAL_FLASH_Unlock();
	FLASH_EraseInitTypeDef EraseInit = { .TypeErase = FLASH_TYPEERASE_PAGES,
			.Page = (FLASH_PAGE_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE, .NbPages =
					1, .Banks = FLASH_BANK };

	uint32_t pageError;
	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&EraseInit, &pageError);
	if (status != HAL_OK) {
		HAL_FLASH_Lock();
		return status;
	}

	for (uint32_t offset = 0; offset < FLASH_PAGE_SIZE; offset += 8) {
		uint64_t word;
		memcpy(&word, &pageBuffer[offset], sizeof(word));
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
		FLASH_PAGE_ADDR + offset, word);

		if (status != HAL_OK)
			break;
	}

	HAL_FLASH_Lock();
	return status;
}

//TODO: Dać w forze wskaźnik i inkrementować o NODE_DATA_SIZE
uint8_t FLASH_NODE_UID_get(STM32_UID_t *stm32_uid, uint8_t node_id) {
	if (stm32_uid == NULL)
		return 0;

	uint8_t offset;
	for (uint8_t i = 0; i < *(uint8_t*) NODE_COUNT_ADDR; ++i) {
		offset = i * NODE_DATA_SIZE;
		if (*((uint8_t*) (NODE_ID_ADDR + offset)) == node_id
				&& check_UID_ID_flag(i)) {

			*stm32_uid = *((STM32_UID_t*) (NODE_UID_ADDR + offset));
			return 1;
		}
	}

	return 0;
}

// Returns node ID. 0 if not found
uint8_t FLASH_NODE_ID_by_uid_get(STM32_UID_t *stm32_uid) {
	if (stm32_uid == NULL)
		return 0;

	uint8_t offset;
	for (uint8_t i = 0; i < *(uint8_t*) NODE_COUNT_ADDR; ++i) {
		offset = i * NODE_DATA_SIZE;
		if (memcmp((void*) (NODE_UID_ADDR + offset), stm32_uid,
				sizeof(STM32_UID_t)) == 0 && check_UID_ID_flag(i)) {

			return *((uint8_t*) (NODE_ID_ADDR + offset));
		}
	}

	return 0;
}

uint8_t FLASH_NODE_ID_get(uint8_t n) {
	if (n >= *(uint8_t*) NODE_COUNT_ADDR)
		return 0;

	if (check_UID_ID_flag(n))
		return *((uint8_t*) (NODE_ID_ADDR + n * NODE_DATA_SIZE));

	return 0;
}

// Returns assigned Node ID
uint8_t FLASH_NODE_UID_ID_add(STM32_UID_t *stm32_uid, uint8_t assigned_id) {
	if (stm32_uid == NULL)
		return 0;

	int8_t uid_pos = find_uid(stm32_uid);
	if (uid_pos >= 0) {
		if (uid_pos == find_id(assigned_id))
			return 1;
		else
			return 0;
	}

	uint8_t pageBuffer[RESERVED_MEMORY_SIZE];
	memcpy(pageBuffer, (uint8_t*) NODE_DATA_ADDR, RESERVED_MEMORY_SIZE);

	uint8_t node_count = pageBuffer[NODE_COUNT_ADDR - NODE_DATA_ADDR];
	if (node_count >= NODE_MAX_COUNT)
		return 0;
	uint16_t node_offset = node_count * NODE_DATA_SIZE;

	memcpy(&pageBuffer[node_offset], (uint8_t*) stm32_uid, sizeof(STM32_UID_t));
	pageBuffer[node_offset + 12] = VALID_FLAG;

	pageBuffer[node_offset + NODE_UID_SIZE] = assigned_id;
	pageBuffer[node_offset + NODE_UID_SIZE + 1] = VALID_FLAG;

	pageBuffer[NODE_COUNT_OFFSET] = node_count + 1;
	pageBuffer[NODE_COUNT_FLAG_OFFSET] = VALID_FLAG;

	if (flash_write(NODE_DATA_ADDR, pageBuffer, RESERVED_MEMORY_SIZE) != HAL_OK)
		return 0;

	return assigned_id;
}

uint8_t FLASH_NODE_ID_update(uint8_t old_id, uint8_t new_id) {
	if (old_id == new_id)
		return 1;

	if (find_id(new_id) >= 0)
		return 0;

	int8_t id_n = find_id(old_id);
	if (id_n < 0)
		return 0;

	uint8_t pageBuffer[RESERVED_MEMORY_SIZE];
	memcpy(pageBuffer, (uint8_t*) NODE_DATA_ADDR, RESERVED_MEMORY_SIZE);

	uint16_t node_offset = id_n * NODE_DATA_SIZE;

	pageBuffer[node_offset + NODE_UID_SIZE] = new_id;
	pageBuffer[node_offset + NODE_UID_SIZE + 1] = VALID_FLAG;

	if (flash_write(NODE_DATA_ADDR, pageBuffer, RESERVED_MEMORY_SIZE) != HAL_OK)
		return 0;

	return 1;
}

uint8_t FLASH_NODE_ID_remove(uint8_t id) {

	int8_t id_n = find_id(id);
	if (id_n < 0)
		return 0;

	uint8_t pageBuffer[RESERVED_MEMORY_SIZE];
	memcpy(pageBuffer, (uint8_t*) NODE_DATA_ADDR, RESERVED_MEMORY_SIZE);

	uint8_t node_count = pageBuffer[NODE_COUNT_OFFSET];
	if (node_count >= NODE_MAX_COUNT)
		return 0;
	uint16_t node_offset = id_n * NODE_DATA_SIZE;

	if (id_n != node_count - 1) {
		uint8_t move_count = node_count - id_n - 1;

		memmove(&pageBuffer[node_offset],
				&pageBuffer[node_offset + NODE_DATA_SIZE],
				NODE_DATA_SIZE * move_count);
	}

	memset(&pageBuffer[(node_count - 1) * NODE_DATA_SIZE], 0, NODE_DATA_SIZE);

	// Count
	pageBuffer[NODE_COUNT_OFFSET] = node_count - 1;
	pageBuffer[NODE_COUNT_OFFSET + 1] = VALID_FLAG;

	if (flash_write(NODE_DATA_ADDR, pageBuffer, RESERVED_MEMORY_SIZE) != HAL_OK)
		return 0;

	return 1;
}

uint8_t FLASH_NODE_count_get(void) {
	return *(uint8_t*) NODE_COUNT_ADDR;
}

uint16_t FLASH_NODE_COMM_WAKEUP_TIMER_INTERVAL_get(uint8_t node_id) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	return *(uint16_t*) (NODE_DATA_ADDR + n * NODE_DATA_SIZE
			+ NODE_COMM_WAKEUP_TIMER_INTERVAL_OFFSET);
}

uint8_t FLASH_NODE_COMM_WAKEUP_TIMER_INTERVAL_set(uint8_t node_id,
		uint16_t value) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	uint8_t buffer[NODE_COMM_WAKEUP_TIMER_INTERVAL_SIZE];
	memcpy(buffer, &value, sizeof(uint16_t));
	buffer[NODE_COMM_WAKEUP_TIMER_INTERVAL_SIZE - 1] = VALID_FLAG;
	uint32_t write_addr = NODE_COMM_WAKEUP_TIMER_INTERVAL_ADDR
			+ n * NODE_DATA_SIZE;

	if (flash_write(write_addr, buffer, NODE_COMM_WAKEUP_TIMER_INTERVAL_SIZE)
			!= HAL_OK)
		return 0;

	return 1;
}

uint8_t FLASH_NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_get(uint8_t node_id) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	return *(uint8_t*) (NODE_DATA_ADDR + n * NODE_DATA_SIZE
			+ NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_OFFSET);
}

uint8_t FLASH_NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_set(uint8_t node_id,
		uint8_t value) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	uint8_t buffer[NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_SIZE];
	memcpy(buffer, &value, sizeof(uint8_t));
	buffer[NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_SIZE - 1] = VALID_FLAG;
	uint32_t write_addr = NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_ADDR
			+ n * NODE_DATA_SIZE;

	if (flash_write(write_addr, buffer, NODE_COMM_WAKEUP_TIMER_TIME_AWAKE_SIZE)
			!= HAL_OK)
		return 0;

	return 1;
}

uint16_t FLASH_NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_get(uint8_t node_id) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	return *(uint16_t*) (NODE_DATA_ADDR + n * NODE_DATA_SIZE
			+ NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_OFFSET);
}

uint8_t FLASH_NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_set(uint8_t node_id,
		uint16_t value) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	uint8_t buffer[NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_SIZE];
	memcpy(buffer, &value, sizeof(uint16_t));
	buffer[NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_SIZE - 1] = VALID_FLAG;
	uint32_t write_addr = NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_ADDR
			+ n * NODE_DATA_SIZE;

	if (flash_write(write_addr, buffer,
			NODE_MEASUREMENT_WAKEUP_TIMER_INTERVAL_SIZE) != HAL_OK)
		return 0;

	return 1;
}

uint8_t FLASH_NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get(uint8_t node_id) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	return *(uint8_t*) (NODE_DATA_ADDR + n * NODE_DATA_SIZE
			+ NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_OFFSET);
}

uint8_t FLASH_NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set(uint8_t node_id,
		uint8_t value) {
	int8_t n = find_id(node_id);
	if (n < 0)
		return 0;

	uint8_t buffer[NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_SIZE];
	memcpy(buffer, &value, sizeof(uint8_t));
	buffer[NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_SIZE - 1] = VALID_FLAG;
	uint32_t write_addr = NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_ADDR
			+ n * NODE_DATA_SIZE;

	if (flash_write(write_addr, buffer,
			NODE_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_SIZE) != HAL_OK)
		return 0;

	return 1;
}

HAL_StatusTypeDef link_config(void) {
	HAL_StatusTypeDef status;

	if (*((uint8_t*) NODE_COUNT_FLAG_ADDR) != VALID_FLAG) {
		uint8_t buffer[2] = { 0, VALID_FLAG };
		status = flash_write(NODE_COUNT_ADDR, buffer, 2);
		if (status != HAL_OK)
			return status;
	}

	return HAL_OK;
}
