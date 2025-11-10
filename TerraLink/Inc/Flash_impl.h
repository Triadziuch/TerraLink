#include "stm32l4xx_hal_flash.h"
#include "stdbool.h"

#define FLASH_BANK          FLASH_BANK_2
#define FLASH_START_ADDR    (FLASH_BASE + FLASH_BANK_SIZE)
#define FLASH_END_ADDR      FLASH_BANK2_END
