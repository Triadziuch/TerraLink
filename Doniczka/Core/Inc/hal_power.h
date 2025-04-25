/*
 * hal_power.h
 *
 *  Created on: Apr 25, 2025
 *      Author: kurow
 */

#ifndef INC_HAL_POWER_H_
#define INC_HAL_POWER_H_

#include "stm32l0xx_hal.h"
#include "SX1278_hw.h"
#include "stdint.h"
#include "stdbool.h"


void POWER_ConfigUnusedPinsAsAnalog(GPIO_TypeDef *ports[], uint16_t pin_masks[], uint8_t count);
void POWER_DisableUnusedPeripherals(void);
void POWER_ConfigLoraPins(const SX1278_hw_t *hw, uint32_t mode, uint32_t pull);
void POWER_RestoreLoraPins(const SX1278_hw_t *hw);
void POWER_EnterStop(void);
void POWER_ExitStop(const SX1278_hw_t *lora_hw);
void POWER_PrepareForSleep(const SX1278_hw_t *lora_hw);
void POWER_GoToSleep(const SX1278_hw_t *lora_hw);

#endif /* INC_HAL_POWER_H_ */
