/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdbool.h"
#include "comm.h"
#include "BH1750.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

#define EXTI_LINE EXTI0_1_IRQn

extern volatile bool rtc_wakeup_flag;

extern ADC_HandleTypeDef hadc;
extern CRC_HandleTypeDef hcrc;
extern RTC_HandleTypeDef hrtc;
extern SPI_HandleTypeDef hspi1;
extern RTC_TimeTypeDef clock_time;
extern RTC_DateTypeDef clock_date;

extern uint8_t lora_buffer[128];
extern volatile uint8_t lora_data_ready;

extern SX1278_hw_t sx1278_hw;
extern SX1278_t sx1278;

extern BH1750_device_t *bh1750;

//extern BH1750_device_t* test_dev;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void SystemClock_Config(void);

/* USER CODE BEGIN EFP */

void ReinitPeripheralsAfterWakeup(void);


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LORA_DIO0_Pin GPIO_PIN_1
#define LORA_DIO0_GPIO_Port GPIOA
#define LORA_DIO0_EXTI_IRQn EXTI0_1_IRQn
#define LORA_RST_Pin GPIO_PIN_3
#define LORA_RST_GPIO_Port GPIOA
#define LORA_NSS_Pin GPIO_PIN_4
#define LORA_NSS_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
