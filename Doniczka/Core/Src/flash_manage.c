#include <flash_manage.h>

uint16_t COMM_WAKEUP_TIMER_INTERVAL = 15,
		MEASUREMENT_WAKEUP_TIMER_INTERVAL = 60;
uint8_t COMM_WAKEUP_TIMER_TIME_AWAKE = 3, MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE =
		3;
uint8_t HIVE_ID = 255;
uint8_t NODE_ID = 0;

void FLASH_NODE_UID_get(STM32_UID_t *stm32_uid) {
	if (stm32_uid != NULL)
		*stm32_uid = *((STM32_UID_t*) NODE_UID_ADDR);
}

uint8_t FLASH_NODE_ID_get(void) {
	return *(uint8_t*) NODE_ID_ADDR;
}

uint8_t FLASH_NODE_ID_set(uint8_t new_id) {

	if (FLASH_NODE_ID_get() == new_id){
		NODE_ID = new_id;
		return 1;
	}

	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	// Set Node ID
	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
	NODE_ID_ADDR, new_id);

	// Set Node ID flag already set
	if (status == HAL_OK) {
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		NODE_ID_FLAG_ADDR, NODE_ID_VALID_FLAG);
	} else {
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	NODE_ID = new_id;
	return 1;
}

uint8_t FLASH_NODE_ID_is_valid(void) {
	return (*(uint8_t*) NODE_ID_FLAG_ADDR == NODE_ID_VALID_FLAG);
}

uint8_t FLASH_HIVE_ID_get(void) {
	return *(uint8_t*) ADDR_HIVE_ID;
}

uint8_t FLASH_HIVE_ID_set(uint8_t hive_id) {
	if (FLASH_HIVE_ID_get() == hive_id && FLASH_HIVE_ID_is_valid()) {
		HIVE_ID = hive_id;
		return 1;
	}

	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
	ADDR_HIVE_ID, hive_id);

	if (status == HAL_OK) {
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		ADDR_HIVE_ID_FLAG, NODE_ID_VALID_FLAG);
	} else {
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	HIVE_ID = hive_id;
	return 1;
}

uint8_t FLASH_HIVE_ID_is_valid(void) {
	return (*(uint8_t*) ADDR_HIVE_ID_FLAG == NODE_ID_VALID_FLAG);
}

uint8_t FLASH_COMM_WAKEUP_TIMER_INTERVAL_set(uint16_t interval) {
	if (FLASH_COMM_WAKEUP_TIMER_INTERVAL_get() == interval){
		COMM_WAKEUP_TIMER_INTERVAL = interval;
		return 1;
	}

	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_HALFWORD,
	ADDR_COMM_WAKEUP_TIMER_INTERVAL, interval);

	if (status == HAL_OK) {
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		ADDR_COMM_WAKEUP_TIMER_INTERVAL_FLAG, NODE_ID_VALID_FLAG);
	} else {
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	COMM_WAKEUP_TIMER_INTERVAL = interval;
	return 1;
}

uint16_t FLASH_COMM_WAKEUP_TIMER_INTERVAL_get(void) {
	if (FLASH_COMM_WAKEUP_TIMER_INTERVAL_is_valid())
		return *(uint8_t*) ADDR_COMM_WAKEUP_TIMER_INTERVAL;
	return 0;
}

uint8_t FLASH_COMM_WAKEUP_TIMER_INTERVAL_is_valid(void) {
	return (*(uint8_t*) ADDR_COMM_WAKEUP_TIMER_INTERVAL_FLAG
			== NODE_ID_VALID_FLAG);
}

uint8_t FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_set(uint8_t time) {
	if (FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_get() == time){
		COMM_WAKEUP_TIMER_TIME_AWAKE = time;
		return 1;
	}

	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
	ADDR_COMM_WAKEUP_TIMER_TIME_AWAKE, time);

	if (status == HAL_OK) {
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		ADDR_COMM_WAKEUP_TIMER_TIME_AWAKE_FLAG, NODE_ID_VALID_FLAG);
	} else {
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	COMM_WAKEUP_TIMER_TIME_AWAKE = time;
	return 1;
}

uint8_t FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_get(void) {
	if (FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_is_valid())
		return *(uint8_t*) ADDR_COMM_WAKEUP_TIMER_TIME_AWAKE;
	return 0;
}

uint8_t FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_is_valid(void) {
	return (*(uint8_t*) ADDR_COMM_WAKEUP_TIMER_TIME_AWAKE_FLAG
			== NODE_ID_VALID_FLAG);
}

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_set(uint16_t interval) {
	if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_get() == interval){
		MEASUREMENT_WAKEUP_TIMER_INTERVAL = interval;
		return 1;
	}

	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_HALFWORD,
	ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL, interval);

	if (status == HAL_OK) {
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_FLAG, NODE_ID_VALID_FLAG);
	} else {
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	MEASUREMENT_WAKEUP_TIMER_INTERVAL = interval;
	return 1;
}

uint16_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_get(void) {
	if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_is_valid())
		return *(uint8_t*) ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL;
	return 0;
}

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_is_valid(void) {
	return (*(uint8_t*) ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_FLAG
			== NODE_ID_VALID_FLAG);
}

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set(uint8_t time) {
	if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get() == time){
		MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE = time;
		return 1;
	}


	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
	ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE, time);

	if (status == HAL_OK) {
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_FLAG, NODE_ID_VALID_FLAG);
	} else {
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE = time;
	return 1;
}

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get(void) {
	if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_is_valid())
		return *(uint8_t*) ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE;
	return 0;
}

uint8_t FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_is_valid(void) {
	return (*(uint8_t*) ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_FLAG
			== NODE_ID_VALID_FLAG);
}

void node_restart(void) {
	HAL_FLASHEx_DATAEEPROM_Unlock();

	for (uint32_t i = NODE_ID_ADDR;
			i <= ADDR_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_FLAG; ++i) {
		HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE,
		i, 0);
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
}

uint8_t node_config(void) {
	if (FLASH_HIVE_ID_is_valid() == 0) {
		if (!FLASH_HIVE_ID_set(HIVE_ID) || !FLASH_NODE_ID_set(NODE_ID))
			return 0;
	} else {
		HIVE_ID = FLASH_HIVE_ID_get();
		NODE_ID = FLASH_NODE_ID_get();

		if (FLASH_COMM_WAKEUP_TIMER_INTERVAL_is_valid() == 0) {
			if (FLASH_COMM_WAKEUP_TIMER_INTERVAL_set(COMM_WAKEUP_TIMER_INTERVAL)
					== 0)
				return 0;
		} else
			COMM_WAKEUP_TIMER_INTERVAL = FLASH_COMM_WAKEUP_TIMER_INTERVAL_get();

		if (FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_is_valid() == 0) {
			if (FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_set(
					COMM_WAKEUP_TIMER_TIME_AWAKE) == 0)
				return 0;
		} else
			COMM_WAKEUP_TIMER_TIME_AWAKE =
					FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_get();

		if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_is_valid() == 0) {
			if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_set(
					MEASUREMENT_WAKEUP_TIMER_INTERVAL) == 0)
				return 0;
		} else
			MEASUREMENT_WAKEUP_TIMER_INTERVAL =
					FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_get();

		if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_is_valid() == 0) {
			if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set(
					MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE) == 0)
				return 0;
		} else
			MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE =
					FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get();
	}

//	if (FLASH_COMM_WAKEUP_TIMER_INTERVAL_is_valid() == 0) {
//		if (FLASH_COMM_WAKEUP_TIMER_INTERVAL_set(COMM_WAKEUP_TIMER_INTERVAL)
//				== 0)
//			return 0;
//	} else
//		COMM_WAKEUP_TIMER_INTERVAL = FLASH_COMM_WAKEUP_TIMER_INTERVAL_get();
//
//	if (FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_is_valid() == 0) {
//		if (FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_set(COMM_WAKEUP_TIMER_TIME_AWAKE)
//				== 0)
//			return 0;
//	} else
//		COMM_WAKEUP_TIMER_TIME_AWAKE = FLASH_COMM_WAKEUP_TIMER_TIME_AWAKE_get();
//
//	if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_is_valid() == 0) {
//		if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_set(
//				MEASUREMENT_WAKEUP_TIMER_INTERVAL) == 0)
//			return 0;
//	} else
//		MEASUREMENT_WAKEUP_TIMER_INTERVAL =
//				FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_INTERVAL_get();
//
//	if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_is_valid() == 0) {
//		if (FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_set(
//				MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE) == 0)
//			return 0;
//	} else
//		MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE =
//				FLASH_MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE_get();

	return 1;
}
