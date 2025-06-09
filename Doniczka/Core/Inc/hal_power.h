/*
 * hal_power.h
 *
 *  Created on: Apr 25, 2025
 *      Author: kurow
 */

#ifndef INC_HAL_POWER_H_
#define INC_HAL_POWER_H_

#include "stm32l0xx_hal.h"
#include "SX1278.h"
#include "stdint.h"
#include "stdbool.h"

extern RTC_HandleTypeDef hrtc;
extern ADC_HandleTypeDef hadc;
extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;

void POWER_ConfigUnusedPinsAsAnalog(GPIO_TypeDef *ports[], uint16_t pin_masks[], uint8_t count);
void POWER_DisableUnusedPeripherals(void);
void POWER_ConfigLoraPins(SX1278_hw_t *sx_hw, uint32_t mode, uint32_t pull);
void POWER_RestoreLoraPins(SX1278_hw_t *sx_hw);
void POWER_EnterStop(void);
void POWER_ExitStop(SX1278_t *sx);
void POWER_PrepareForSleep(SX1278_hw_t *sx_hw);
void POWER_GoToSleep(SX1278_t *sx);

#endif /* INC_HAL_POWER_H_ */
