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

void POWER_ConfigLoraPins(SX1278_hw_t *sx_hw, uint32_t mode, uint32_t pull){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef init = {
		.Mode = mode,
		.Pull = pull,
		.Speed = GPIO_SPEED_FREQ_LOW
	};

	init.Pin = sx_hw->dio0.pin;
	HAL_GPIO_Init(sx_hw->dio0.port, &init);

	init.Pin = sx_hw->nss.pin;
	HAL_GPIO_Init(sx_hw->nss.port, &init);

	init.Pin = sx_hw->reset.pin;
	HAL_GPIO_Init(sx_hw->reset.port, &init);
}

void POWER_RestoreLoraPins(SX1278_hw_t *sx_hw){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitTypeDef init = {
			.Mode = GPIO_MODE_OUTPUT_PP,
			.Pull = GPIO_NOPULL,
			.Speed = GPIO_SPEED_FREQ_HIGH
	};

	init.Pin = sx_hw->nss.pin;
	HAL_GPIO_Init(sx_hw->nss.port, &init);
	HAL_GPIO_WritePin(sx_hw->nss.port, sx_hw->nss.pin, GPIO_PIN_SET);

	init.Pin = sx_hw->reset.pin;
	HAL_GPIO_Init(sx_hw->reset.port, &init);
	HAL_GPIO_WritePin(sx_hw->reset.port, sx_hw->reset.pin, GPIO_PIN_SET);

	init.Mode = GPIO_MODE_IT_RISING;
	init.Pin = sx_hw->dio0.pin;
	init.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(sx_hw->dio0.port, &init);

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

void POWER_ExitStop(SX1278_t *sx){
	RCC_OscInitTypeDef osc = {0};
	osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	osc.HSIState = RCC_HSI_ON;
	osc.PLL.PLLState = RCC_PLL_NONE;
	HAL_RCC_OscConfig(&osc);

	SystemClock_Config();

	HAL_ResumeTick();

	ReinitPeripheralsAfterWakeup();

	if (sx)
		POWER_RestoreLoraPins(sx->hw);
}


void POWER_PrepareForSleep(SX1278_hw_t *sx_hw){
	uint8_t unused_count = sizeof(UNUSED_GPIO_PIN_MASKS) / sizeof(UNUSED_GPIO_PIN_MASKS[0]);
	POWER_ConfigUnusedPinsAsAnalog(UNUSED_GPIO_PORTS,  UNUSED_GPIO_PIN_MASKS, unused_count);
	POWER_DisableUnusedPeripherals();

	if (sx_hw)
		POWER_ConfigLoraPins(sx_hw, GPIO_MODE_ANALOG, GPIO_NOPULL);
}

void POWER_GoToSleep(SX1278_t *sx){
	POWER_PrepareForSleep(sx->hw);
	POWER_EnterStop();
	POWER_ExitStop(sx);
}
