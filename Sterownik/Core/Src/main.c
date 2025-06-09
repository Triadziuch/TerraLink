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
#include "SX1278.h"
#include "string.h"
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

RTC_TimeTypeDef clock_time;
RTC_DateTypeDef clock_date;

// Lora transmitter
SX1278_hw_t sx1278_hw;
SX1278_t sx1278;

// Lora data variables
uint8_t lora_buffer[128];
volatile uint8_t lora_data_ready = 0;

// Communication test variables
int i;
static uint32_t last_check = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void comm_test() {

	char *data_buffer = "10";
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	SX1278_transmit(&sx1278, (uint8_t*) data_buffer, strlen(data_buffer), 1000);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	printf("10\n");

	if (SX1278_receive(&sx1278, 64, 2000)) {
	} else {
	}

	while (1) {
		if (lora_data_ready) {

			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
			lora_data_ready = false;

			HAL_Delay(10);

			sscanf(lora_buffer, "%d", &i);
			printf("%d\n", i);

			char data_buffer[8];
			sprintf(data_buffer, "%d", ++i);

			bool status = comm_tx((uint8_t*) data_buffer, strlen(data_buffer),
					1000);
			HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

			if (SX1278_receive(&sx1278, 64, 2000))
				if (i) {
				};
			last_check = HAL_GetTick();
		} else {
			if (HAL_GetTick() - last_check > 2000) {
				last_check = HAL_GetTick();

				HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
				SX1278_hw_Reset(sx1278.hw);
				HAL_Delay(100);

				printf("Zrestartowano sterownik!\n");

				sscanf(lora_buffer, "%d", &i);
				printf("%d\n", i);

				char data_buffer[8];
				sprintf(data_buffer, "%d", i);

				bool status = comm_tx((uint8_t*) data_buffer,
						strlen(data_buffer), 1000);
				HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

				if (SX1278_receive(&sx1278, 64, 2000)) {
				}

			}
		}
	}
}

int __io_putchar(int ch) {
	if (ch == '\n') {
		uint8_t ch2 = '\r';
		HAL_UART_Transmit(&huart2, &ch2, 1, HAL_MAX_DELAY);
	}
	HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
	return 1;
}

void init_time(void){
	RTC_TimeTypeDef new_time = {0};

	new_time.Hours = 12;
	new_time.Minutes = 0;
	new_time.Seconds = 0;

	HAL_RTC_SetTime(&hrtc, &new_time, RTC_FORMAT_BIN);
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
	MX_USART2_UART_Init();
	MX_RTC_Init();
	/* USER CODE BEGIN 2 */

	comm_init();

	init_time();

	// Od razu przejście do trybu nasłuchiwania
	printf("Starting LoRa receiver mode...\n");
	if (SX1278_receive(&sx1278, 64, 2000)) {
		printf("LoRa receiver mode activated!\n");
	} else {
		printf("Failed to activate LoRa receiver mode!\n");
	}


	packet_t received_pkt;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		if (lora_data_ready) {
			if (verify_pkt(&received_pkt)) {
				printf("Packet check successful\n");


				if (received_pkt.pkt_type == PKT_REG_REQ){
					comm_handshake_slave(&received_pkt);
					SX1278_receive(&sx1278, 64, 2000);

				}
				else{
					if (received_pkt.pkt_type == PKT_DATA){
						comm_handle_data(&received_pkt);
						SX1278_receive(&sx1278, 64, 2000);
					}

				}

			} else {
				printf("Packet check failed\n");
			}

			lora_data_ready = 0;
		}

		//comm_test();
//	  while (1){
//		  char *data_buffer = "10";
//		  //bool status = comm_tx((uint8_t*)data_buffer, strlen(data_buffer), 1000);
//		  HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
//		  bool status = SX1278_transmit(&sx1278, data_buffer, strlen(data_buffer), 1000);
//		  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
//		  if (status){
//			  printf("Packet sent!\n");
//		  }
//		  else
//			  printf("Packet not sent!!!!!!\n");
//
////		  if (test_comm()){
////			  printf("Packet sent!\n");
////		  }
////		  else
////			  printf("Packet not sent!!!!\n");
//	  }
//    if (lora_data_ready) {
//    	printf("%s\n", lora_buffer);
//      //printf("Received LoRa data: %s\n", lora_buffer);
//      lora_data_ready = 0;
//
//      //printf("Reactivating LoRa receiver mode...\n");
//      if (!SX1278_receive(&sx1278, 64, 2000)) {
//        //printf("Failed to reactivate receiver mode!\n");
//
//        SX1278_hw_Reset(sx1278.hw);
//        HAL_Delay(100);
//
//        SX1278_receive(&sx1278, 64, 2000);
//        //if (SX1278_receive(&sx1278, 64, 2000))
//          //printf("Receiver mode restored after reset\n");
//      }
//    }
//    if (!lora_data_ready){
//    	printf("Entering sleep mode...\n");
//    	HAL_SuspendTick();
//    	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
//    	HAL_ResumeTick();
//    	printf("Woke up from sleep mode.\n");
//    }
		static uint32_t last_check = 0;
		if (HAL_GetTick() - last_check > 15000) {
			last_check = HAL_GetTick();

			if (sx1278.status != RX) {
				printf("LoRa not in RX mode! Current status: %d\n",
						sx1278.status);
				printf("Resetting and reactivating receiver...\n");

				SX1278_hw_Reset(sx1278.hw);
				HAL_Delay(100);

				if (SX1278_receive(&sx1278, 64, 2000))
					printf("Receiver mode restored!\n");
			} else
				printf("LoRa receiver active, status OK\n");
		}

		//HAL_Delay();
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

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI
			| RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 40;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV16;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		Error_Handler();
	}

	/** Enable MSI Auto calibration
	 */
	HAL_RCCEx_EnableMSIPLLMode();
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
	hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

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
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LORA_RST_GPIO_Port, LORA_RST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LORA_NSS_GPIO_Port, LORA_NSS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LORA_RST_Pin */
	GPIO_InitStruct.Pin = LORA_RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LORA_RST_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LORA_DIO0_Pin */
	GPIO_InitStruct.Pin = LORA_DIO0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(LORA_DIO0_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LORA_NSS_Pin */
	GPIO_InitStruct.Pin = LORA_NSS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LORA_NSS_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

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
