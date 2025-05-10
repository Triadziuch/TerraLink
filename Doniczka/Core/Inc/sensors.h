/*
 * sensors.h
 *
 *  Created on: May 10, 2025
 *      Author: kurow
 */

#ifndef INC_SENSORS_H_
#define INC_SENSORS_H_

#include "main.h"

#define ADC_MOISTURE_MAX 3408
#define ADC_MOISTURE_MIN 1379
#define ADC_MOISTURE_RANGE (ADC_MOISTURE_MAX - ADC_MOISTURE_MIN)

extern uint16_t soil_moisture;
extern uint16_t soil_moisture_percentage;

typedef struct{
	uint32_t time;
	uint16_t value;
} sensor_data_raw_t;

int ReadSoilMoistureSensor(uint16_t *soil_moisture);
int ConvertSoilMoistureToPercentage(uint16_t *soil_moisture,
		uint16_t *soil_moisture_percentage);

int GetSoilMoisturePercentage(sensor_data_raw_t *moisture_data);

#endif /* INC_SENSORS_H_ */
