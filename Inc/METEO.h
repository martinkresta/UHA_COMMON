/*
 * METEO.h
 *
 *  Created on: Aug 22, 2022
 *      Author: Martin
 *      Brief: Module for collecting outdoor meteo information (Wind speed sensor, Rainfall sensor)
 */

#ifndef INC_METEO_H_
#define INC_METEO_H_


#include "main.h"
#include "stdlib.h"

#define   MAX_WINDPULSE_PERIOD_MS   	10000
#define   WIND_AVG_WINDOW_S            30

#define   AIR_DENSITY_GM3              1296
#define   TURBINE_EFFICIENCY_PCT       20

typedef struct
{
   uint16_t WindSpeedAvg;
   uint16_t WindSpeedBurst;
   uint64_t EnergyuWh;
   uint16_t PowerW;
   uint32_t LastPulse;
   uint32_t Pin;
   uint16_t VarId;
   uint16_t avgIdx;
   uint16_t buff[WIND_AVG_WINDOW_S];
}sWindSensor;

typedef struct
{
   uint16_t ActualRainFall;
   uint16_t CumulatedRainFall;
   uint32_t LastPulse;
   uint32_t Pin;
   uint16_t VarId;
}sRainSensor;


void METEO_Init(uint32_t windPin, uint32_t rainPin);

void METEO_Update_1s(void);

uint16_t METEO_GetWindSpeedAvg(void);
uint16_t METEO_GetWindSpeedBurst(void);
uint16_t METEO_GetWindEnergyWh(void);
void METEO_ResetWindEnergy(void);
void METEO_ResetRainFall(void);

void METEO_ExtiCallback(uint16_t GPIO_Pin);



#endif /* INC_METEO_H_ */
