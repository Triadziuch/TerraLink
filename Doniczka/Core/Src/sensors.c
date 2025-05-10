/*
 * sensors.c
 *
 *  Created on: May 10, 2025
 *      Author: kurow
 */

#include "sensors.h"
#include "hal_rtc.h"

uint16_t soil_moisture = 0;
uint16_t soil_moisture_percentage = 0;

int ReadSoilMoistureSensor(uint16_t *soil_moisture) {

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
int ConvertSoilMoistureToPercentage(uint16_t *soil_moisture,
		uint16_t *soil_moisture_percentage) {
	if (*soil_moisture >= ADC_MOISTURE_MAX)
		*soil_moisture_percentage = 0; // 0%
	else if (*soil_moisture <= ADC_MOISTURE_MIN)
		*soil_moisture_percentage = 1000; // 100%
	else {
		uint16_t delta = *soil_moisture - ADC_MOISTURE_MIN;
		*soil_moisture_percentage = (uint16_t)(((uint32_t)(ADC_MOISTURE_RANGE - delta) * 1000) / ADC_MOISTURE_RANGE);

		return 1;
	}

	return 0;
}

int GetSoilMoisturePercentage(sensor_data_raw_t *moisture_data){
	if (moisture_data == NULL)
		return 0;

	if (ReadSoilMoistureSensor(&soil_moisture))
		if (ConvertSoilMoistureToPercentage(&soil_moisture, &soil_moisture_percentage)){
			moisture_data->time = GetTime();
			moisture_data->value = soil_moisture_percentage;
			return 1;
		}

	return 0;
}
