/*
 * hal_rtc.h
 *
 *  Created on: Apr 25, 2025
 *      Author: kurow
 */

#ifndef INC_HAL_RTC_H_
#define INC_HAL_RTC_H_

#include "stm32l0xx_hal.h"

HAL_StatusTypeDef HAL_RTC_SetWakeup(uint32_t time_s);
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc);

#endif /* INC_HAL_RTC_H_ */
