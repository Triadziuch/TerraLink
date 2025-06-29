/*
 * hal_rtc.c
 *
 *  Created on: Apr 25, 2025
 *      Author: kurow
 */

#include "hal_rtc.h"
#include "main.h"

HAL_StatusTypeDef HAL_RTC_SetWakeup(uint32_t time_s){
	HAL_StatusTypeDef status = HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

	if (status != HAL_OK)
		return status;

	rtc_wakeup_flag = false;

	return HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, time_s * 2048 - 1, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc_ptr){
	rtc_wakeup_flag = true;
}
