#include "stm32l0xx_hal_flash.h"
#include "stdbool.h"

// For STM32L031 (32KB Flash), use the end of Flash memory
// FLASH_BASE = 0x08000000, FLASH_SIZE = 32KB = 0x8000
#define FLASH_START_ADDR    (FLASH_BASE + (FLASH_SIZE / 2))  // Start from middle of Flash
#define FLASH_END_ADDR      FLASH_END                         // End of Flash (uses HAL definition)

