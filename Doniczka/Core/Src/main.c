/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "SX1278.h"
#include "string.h"
#include "hal_rtc.h"
#include "hal_power.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ADC_MOISTURE_MAX 3408
#define ADC_MOISTURE_MIN 1379
#define ADC_MOISTURE_RANGE (ADC_MOISTURE_MAX - ADC_MOISTURE_MIN)
#define RTC_WAKEUP_TIME_S 3

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

// Lora transmitter
SX1278_hw_t sx1278_hw;
SX1278_t sx1278;

// Lora data variables
uint8_t lora_buffer[128];
volatile uint8_t lora_data_ready = 0;

// Communication test variables
int i;
static uint32_t last_check = 0;

// RTC Timer variables
volatile bool rtc_wakeup_flag = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void comm_test(void) {

	if (SX1278_receive(&sx1278, 64, 2000)) {
	} else {
	}

	while (1) {
		if (lora_data_ready) {
			HAL_NVIC_DisableIRQ(EXTI_LINE);
			lora_data_ready = false;

			HAL_Delay(10);

			sscanf(lora_buffer, "%d", &i);
			printf("%d\n", i);

			char data_buffer[8];
			sprintf(data_buffer, "%d", ++i);

			bool status = comm_tx((uint8_t*) data_buffer, strlen(data_buffer),
					1000);
			HAL_NVIC_EnableIRQ(EXTI_LINE);

			if (SX1278_receive(&sx1278, 64, 2000))
				if (i) {
				};
			last_check = HAL_GetTick();

		} else {

			if (HAL_GetTick() - last_check > 2000) {
				last_check = HAL_GetTick();

				HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
				SX1278_hw_Reset(sx1278.hw);
				HAL_Delay(100);

				sscanf(lora_buffer, "%d", &i);
				printf("%d\n", i);

				char data_buffer[8];
				i += 100000;
				sprintf(data_buffer, "%d", i);

				bool status = comm_tx((uint8_t*) data_buffer,
						strlen(data_buffer), 1000);
				HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

				if (SX1278_receive(&sx1278, 64, 2000))
					if (i) {
					};
			}
		}
	}

}

bool ReadSoilMoistureSensor(uint16_t *soil_moisture) {

	uint16_t adc_value = { 0 };
	uint8_t adc_channels_read = 0;

	HAL_ADC_Start(&hadc);

	if (HAL_ADC_PollForConversion(&hadc, 200) == HAL_OK) {
		adc_value = HAL_ADC_GetValue(&hadc);
		adc_channels_read++;
	}

	HAL_ADC_Stop(&hadc);

	if (adc_channels_read == 1) {
		*soil_moisture = adc_value;
		return 1;
	}

	return 0;
}

// Zakres 0-200, procent = soil_moisture_percentage * 0.5
bool ConvertSoilMoistureToPercentage(uint16_t *soil_moisture,
		uint8_t *soil_moisture_percentage) {
	if (*soil_moisture >= ADC_MOISTURE_MAX)
		*soil_moisture_percentage = 0; // 0%
	else if (*soil_moisture <= ADC_MOISTURE_MIN)
		*soil_moisture_percentage = 200; // 100%
	else {
		uint16_t delta = ADC_MOISTURE_MAX - *soil_moisture;
		*soil_moisture_percentage = (delta * 200 + (ADC_MOISTURE_RANGE / 2))
				/ ADC_MOISTURE_RANGE;
		return 1;
	}
	return 0;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_SPI1_Init();
	MX_ADC_Init();
	MX_RTC_Init();
	/* USER CODE BEGIN 2 */

	// SX1278 module initialization
	comm_init();

	// ADC and Sensor Initialization
	HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);
	uint16_t soil_moisture = 0;
	uint8_t soil_moisture_percentage = 0;

	// RTC
	__HAL_RCC_RTC_ENABLE();

	if (HAL_RTC_SetWakeup(RTC_WAKEUP_TIME_S) != HAL_OK) {
		Error_Handler();
	}

	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	bool handshake = false;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		while (handshake == false){
			handshake = handshake_master();
		}

		//comm_test();
//	  packet_t packet;
//	  comm_receive(&packet, 3000);

//	  POWER_GoToSleep(&sx1278);

//	  static uint32_t last_check = 0;
//	  if (HAL_GetTick() - last_check > 15000) {
//		last_check = HAL_GetTick();
//
//		if (sx1278.status != RX) {
//		  printf("LoRa not in RX mode! Current status: %d\n", sx1278.status);
//		  printf("Resetting and reactivating receiver...\n");
//
//		  SX1278_hw_Reset(sx1278.hw);
//		  HAL_Delay(100);
//
//		  if (SX1278_receive(&sx1278, 64, 2000))
//			printf("Receiver mode restored!\n");
//		}
//		else
//		  printf("LoRa receiver active, status OK\n");
//	  }

//	  if (rtc_wakeup_flag){
//		  rtc_wakeup_flag = false;
//
//		  if (ReadSoilMoistureSensor(&soil_moisture)){
//		  		  char data_buffer[64];
//		  		  ConvertSoilMoistureToPercentage(&soil_moisture, &soil_moisture_percentage);
//
//		  		  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
//		  		  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
//
//		  		  sprintf(data_buffer, "Czas: %02d:%02d:%02d\tMoisture: %u.%u%% ADC = %u\n", time.Hours, time.Minutes, time.Seconds, soil_moisture_percentage / 2, (soil_moisture_percentage % 2) * 5, soil_moisture);
//		  		  HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
//		  		  bool status = comm_tx((uint8_t*)data_buffer, strlen(data_buffer), 8000);
//		  		  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
//		  		  if (status)
//		  			  printf("Wyslano\r\n");
//		  		  else
//		  			  printf("Nie wyslano\r\n");
//		  	  }
//		  	  else{
//		  		  uint8_t message[] = "Wystapil blad!";
//		  		  bool status = comm_tx(message, sizeof(message), 1000);
//		  	  }
//	  }

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief ADC Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC_Init(void) {

	/* USER CODE BEGIN ADC_Init 0 */

	/* USER CODE END ADC_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC_Init 1 */

	/* USER CODE END ADC_Init 1 */

	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc.Instance = ADC1;
	hadc.Init.OversamplingMode = DISABLE;
	hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
	hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.DMAContinuousRequests = DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc.Init.LowPowerAutoWait = DISABLE;
	hadc.Init.LowPowerFrequencyMode = DISABLE;
	hadc.Init.LowPowerAutoPowerOff = DISABLE;
	if (HAL_ADC_Init(&hadc) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel to be converted.
	 */
	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC_Init 2 */

	/* USER CODE END ADC_Init 2 */

}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */

	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}

	/** Enable the WakeUp
	 */
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 7;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LORA_RST_Pin | LORA_NSS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LORA_DIO0_Pin */
	GPIO_InitStruct.Pin = LORA_DIO0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(LORA_DIO0_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LORA_RST_Pin LORA_NSS_Pin */
	GPIO_InitStruct.Pin = LORA_RST_Pin | LORA_NSS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  EXTI line detection callback.
 * @param  GPIO_Pin Specifies the port pin connected to corresponding EXTI line.
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == LORA_DIO0_Pin) {
		// Handle LoRa interrupt
		uint8_t received_data[128];
		uint8_t data_length = 0;

		if (SX1278_available(&sx1278) && !lora_data_ready) {

			data_length = SX1278_read(&sx1278, received_data, 64);
			if (data_length > 0) {
				received_data[data_length] = '\0';

				// Kopiowanie danych do bufora globalnego
				memcpy(lora_buffer, received_data, data_length + 1);
				lora_data_ready = 1;
			}
		}
	}
}

void ReinitPeripheralsAfterWakeup(void) {
	MX_ADC_Init();
	MX_SPI1_Init();
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
