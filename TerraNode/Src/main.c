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

#define RTC_WAKEUP_TIME_S 3

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

// Lora transmitter
SX1278_hw_t sx1278_hw;
SX1278_t sx1278;

// BH1750
BH1750_device_t *bh1750;

// Lora data variables
uint8_t lora_buffer[128];
volatile uint8_t lora_data_ready = 0;

// Communication test variables removed

// RTC Timer variables
volatile bool rtc_wakeup_flag = false;
wakeup_t *wakeup_info;
bool handshake = false;

// RTC Time and Date variables
RTC_TimeTypeDef clock_time;
RTC_DateTypeDef clock_date;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C1_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void handle_received_pkt(packet_t *received_pkt) {
	if (received_pkt->pkt_type == PKT_REQ_DATA) {
		comm_handle_req_data(received_pkt);
	} else if (received_pkt->pkt_type == PKT_TEST_CONN) {
		comm_handle_test_conn(received_pkt);
	} else if (received_pkt->pkt_type == PKT_ASSIGN_ID) {

	} else if (received_pkt->pkt_type == PKT_REQ_ID) {

	} else if (received_pkt->pkt_type == PKT_CMD) {
		comm_handle_cmd(received_pkt);
	}
}

void handle_measurement() {
	comm_send_moisture(NULL);
	comm_send_lux(NULL);
}

void handle_communication(uint8_t awake_time) {
	uint32_t time = HAL_GetTick();
	while (HAL_GetTick() - time >= awake_time * 1000) {
		time = HAL_GetTick();

		SX1278_receive(&sx1278, 64, 1000);
		if (lora_data_ready) {
			packet_t *received_pkt = packet_alloc(MAX_PAYLOAD_SIZE);
			if (received_pkt != NULL && verify_pkt(received_pkt)) {
				switch (received_pkt->pkt_type) {
				case PKT_REQ_DATA:
					comm_handle_req_data(received_pkt);
					break;
				case PKT_TEST_CONN:
					comm_handle_test_conn(received_pkt);
					break;
				case PKT_CMD:
					comm_handle_cmd(received_pkt);
					break;
				case PKT_ASSIGN_ID:
					break;
				case PKT_REQ_ID:
					break;
				}
			}
			if (received_pkt != NULL)
				packet_free(received_pkt);
			lora_data_ready = 0;
		}
	}
}

void handle_wakeup() {
	if (wakeup_info->wakeup_type == MEASUREMENT) {
		handle_measurement();
		handle_communication(MEASUREMENT_WAKEUP_TIMER_TIME_AWAKE);
	} else {
		handle_communication(COMM_WAKEUP_TIMER_TIME_AWAKE);
	}

	free(wakeup_info);
	while ((wakeup_info = comm_send_next_wakeup()) == NULL) {
	}
	HAL_RTC_SetWakeup(wakeup_info->time);
	POWER_GoToSleep(&sx1278);
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
	MX_I2C1_Init();
	MX_CRC_Init();
	/* USER CODE BEGIN 2 */

	// SX1278 module initialization
	comm_init();

	// ADC and Sensor Initialization
	HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);

	// RTC
	__HAL_RCC_RTC_ENABLE();

	// I2C
	bh1750 = BH1750_init_dev_struct(&hi2c1, "bh1750", true);
	BH1750_init_dev(bh1750);

	// CRC Init
	__HAL_RCC_CRC_CLK_ENABLE();

	if (node_config() == 0)
		Error_Handler();

	// Validate Hive Connection
	while (handshake == false) {
		handshake = comm_handshake_master();
		SX1278_receive(&sx1278, 64, 500);
	}

	while (!comm_await_start()) {
	}

	wakeup_info = malloc(sizeof(wakeup_t));
	wakeup_info->wakeup_type = MEASUREMENT;
	wakeup_info->time = MEASUREMENT_WAKEUP_TIMER_INTERVAL;
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		handle_wakeup();
//		if (lora_data_ready) {
//			if (verify_pkt(&received_pkt)) {
//				printf("Packet check successful\n");
//
//				if (received_pkt.pkt_type == PKT_REQ_DATA) {
//					comm_handle_req_data(&received_pkt);
//				} else if (received_pkt.pkt_type == PKT_TEST_CONN) {
//					comm_handle_test_conn(&received_pkt);
//				} else if (received_pkt.pkt_type == PKT_ASSIGN_ID) {
//
//				} else if (received_pkt.pkt_type == PKT_REQ_ID) {
//
//				} else if (received_pkt.pkt_type == PKT_CMD) {
//					comm_handle_cmd(&received_pkt);
//				}
//
//				SX1278_receive(&sx1278, 64, 2000);
//
//			} else {
//				printf("Packet check failed\n");
//			}
//
//			lora_data_ready = 0;
//		}

//		POWER_GoToSleep(&sx1278);

//		static uint32_t last_check = 0;
//		if (HAL_GetTick() - last_check > 15000) {
//			last_check = HAL_GetTick();
//
//			if (sx1278.status != RX) {
//				SX1278_hw_Reset(sx1278.hw);
//				HAL_Delay(100);
//
//				SX1278_receive(&sx1278, 64, 2000);
//			}
//		}

//		if (rtc_wakeup_flag) {
//			rtc_wakeup_flag = false;
//
//			if (comm_send_moisture()) {
//			} else {
//			}
//
//			if (comm_send_lux()) {
//
//			} else {
//			}
//		}
//

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
			| RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
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
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void) {

	/* USER CODE BEGIN CRC_Init 0 */

	/* USER CODE END CRC_Init 0 */

	/* USER CODE BEGIN CRC_Init 1 */

	/* USER CODE END CRC_Init 1 */
	hcrc.Instance = CRC;
	hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
	hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
	hcrc.Init.GeneratingPolynomial = 7;
	hcrc.Init.CRCLength = CRC_POLYLENGTH_16B;
	hcrc.Init.InitValue = 65535;
	hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
	if (HAL_CRC_Init(&hcrc) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN CRC_Init 2 */

	/* USER CODE END CRC_Init 2 */

}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x00201D2B;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

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

	/* Enable the WakeUp */
//	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16)
//			!= HAL_OK) {
//		Error_Handler();
//	}
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
	MX_I2C1_Init();
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
