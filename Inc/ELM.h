/*
 * ELM.h
 *
 *  Created on: Nov 22, 2021
 *      Author: Martin
 *      Brief: Module measuring AC power consumption and actual power.  Using EXTI gpio inputs
 */

#ifndef INC_ELM_H_
#define INC_ELM_H_


#include "main.h"
#include "stdlib.h"

#define   MIN_MEASURABLE_POWER_W		10

typedef struct
{
	uint8_t id;
	uint16_t Wh;
	uint16_t W;
	uint32_t LastPulse;
	uint32_t Pin;
	uint16_t VarId;
}sElMeter;


void ELM_Init(uint8_t numOfMeters);

void ELM_Update_1s(void);

void ELM_AddMeter(uint8_t ElmId, uint32_t pin, uint16_t VarId);

uint16_t ELM_GetConsumptionWh(uint8_t ElmId);
uint16_t ELM_GetPowerW(uint8_t ElmId);
void ELM_ResetConsumption(uint8_t ElmId);
void ELM_ResetAllConsumptions(void);

void ELM_ExtiCallback(uint16_t GPIO_Pin);



#endif /* INC_ELM_H_ */
