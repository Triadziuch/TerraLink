/*
 * hal_power.c
 *
 *  Created on: Apr 25, 2025
 *      Author: kurow
 */

#include "hal_power.h"
#include "main.h"

extern RTC_HandleTypeDef hrtc;
extern ADC_HandleTypeDef hadc;
extern SPI_HandleTypeDef hspi1;

GPIO_TypeDef *UNUSED_GPIO_PORTS[] = {
		GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA,
		GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB
};

uint16_t UNUSED_GPIO_PIN_MASKS[] = {
		GPIO_PIN_7, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10,
		GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_15,
		GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_3, GPIO_PIN_4,
		GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7
};

void POWER_ConfigUnusedPinsAsAnalog(GPIO_TypeDef *ports[], uint16_t pin_masks[], uint8_t count){
	for (uint8_t i = 0; i < count; ++i){
		GPIO_TypeDef *port = ports[i];
		uint16_t pins = pin_masks[i];

		if (port == GPIOA)
			__HAL_RCC_GPIOA_CLK_ENABLE();
		else if (port == GPIOB)
			__HAL_RCC_GPIOB_CLK_ENABLE();

		GPIO_InitTypeDef init = {
				.Pin = pins,
				.Mode = GPIO_MODE_ANALOG,
				.Pull = GPIO_NOPULL
		};

		HAL_GPIO_Init(port, &init);
	}
}

void POWER_DisableUnusedPeripherals(void){
	HAL_ADC_DeInit(&hadc);
	HAL_SPI_DeInit(&hspi1);
}

void POWER_ConfigLoraPins(const SX1278_hw_t *hw, uint32_t mode, uint32_t pull){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef init = {
		.Mode = mode,
		.Pull = pull,
		.Speed = GPIO_SPEED_FREQ_LOW
	};

	init.Pin = hw->dio0.pin;
	HAL_GPIO_Init(hw->dio0.port, &init);

	init.Pin = hw->nss.pin;
	HAL_GPIO_Init(hw->nss.port, &init);

	init.Pin = hw->reset.pin;
	HAL_GPIO_Init(hw->reset.port, &init);
}

void POWER_RestoreLoraPins(const SX1278_hw_t *hw){
}

void POWER_EnterStop(void){
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();

	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_SuspendTick();
	__disable_irq();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	__enable_irq();
}

void POWER_ExitStop(const SX1278_hw_t *lora_hw){
	RCC_OscInitTypeDef osc = {0};
	osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	osc.HSIState = RCC_HSI_ON;
	osc.PLL.PLLState = RCC_PLL_NONE;
	HAL_RCC_OscConfig(&osc);

	extern void SystemClockConfig(void);
	SystemClock_Config();

	HAL_ResumeTick();

	ReinitPeripheralsAfterWakeup();

	if (lora_hw){
//		SX1278_hw_t *hw = (SX1278_hw_t *)lora_hw;
//		SX1278_init(&sx1278,
//					433000000,
//					SX1278_POWER_17DBM,
//					SX1278_LORA_SF_7,
//					SX1278_LORA_BW_125KHZ,
//					SX1278_LORA_CR_4_5,
//					SX1278_LORA_CRC_EN,
//					64);
	}
}


void POWER_PrepareForSleep(const SX1278_hw_t *lora_hw){
	uint8_t unused_count = sizeof(UNUSED_GPIO_PIN_MASKS) / sizeof(UNUSED_GPIO_PIN_MASKS[0]);
	POWER_ConfigUnusedPinsAsAnalog(UNUSED_GPIO_PORTS,  UNUSED_GPIO_PIN_MASKS, unused_count);
	POWER_DisableUnusedPeripherals();

	if (lora_hw)
		POWER_ConfigLoraPins(lora_hw, GPIO_MODE_ANALOG, GPIO_NOPULL);
}

void POWER_GoToSleep(const SX1278_hw_t *lora_hw){
	POWER_PrepareForSleep(lora_hw);
	POWER_EnterStop();
	POWER_ExitStop(lora_hw);
}
