#include <flash_manage.h>

void FLASH_NODE_UID_get(STM32_UID_t *stm32_uid){
	if (stm32_uid != NULL)
		*stm32_uid = *((STM32_UID_t*)NODE_UID_ADDR);
}

uint8_t FLASH_NODE_ID_get(void){
	return *(uint8_t*)NODE_ID_ADDR;
}

bool FLASH_NODE_ID_set(uint8_t new_id){
	
	if (FLASH_NODE_ID_get() == new_id)
		return 1;

	HAL_StatusTypeDef status;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	// Set Node ID
	status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, NODE_ID_ADDR, new_id);

	// Set Node ID flag already set
	if (status == HAL_OK){
		status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, NODE_ID_FLAG_ADDR, NODE_ID_VALID_FLAG);
	}
	else{
		HAL_FLASHEx_DATAEEPROM_Lock();
		return 0;
	}

	HAL_FLASHEx_DATAEEPROM_Lock();
	return 1;
}

bool FLASH_NODE_ID_is_valid(void){
	return (*(uint8_t*)NODE_ID_FLAG_ADDR == NODE_ID_VALID_FLAG);
}
