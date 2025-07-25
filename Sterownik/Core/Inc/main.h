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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "packet.h"
#include "comm.h"


/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

#define EXTI_LINE EXTI9_5_IRQn

extern RTC_HandleTypeDef hrtc;
extern CRC_HandleTypeDef hcrc;
extern SPI_HandleTypeDef hspi1;

extern RTC_TimeTypeDef clock_time;
extern RTC_DateTypeDef clock_date;

extern uint8_t lora_buffer[128];
extern volatile uint8_t lora_data_ready;

extern SX1278_hw_t sx1278_hw;
extern SX1278_t sx1278;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LORA_RST_Pin GPIO_PIN_7
#define LORA_RST_GPIO_Port GPIOC
#define LORA_DIO0_Pin GPIO_PIN_9
#define LORA_DIO0_GPIO_Port GPIOA
#define LORA_DIO0_EXTI_IRQn EXTI9_5_IRQn
#define LORA_NSS_Pin GPIO_PIN_6
#define LORA_NSS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
