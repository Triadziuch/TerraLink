#ifndef FLASH_H
#define FLASH_H

#include "stm32l0xx_hal.h"

HAL_StatusTypeDef Flash_read(uint32_t address, uint8_t *data, uint16_t length);
HAL_StatusTypeDef Flash_read_check_flag(uint32_t address, uint8_t *data, uint16_t length);
HAL_StatusTypeDef Flash_write(uint32_t address, const uint8_t *data, uint16_t length);

#endif // FLASH_H

