/*
 * ELM.c
 *
 *  Created on: Nov 22, 2021
 *      Author: Martin
 *      Brief: Module measuring AC power consumption and actual power.  Using EXTI gpio inputs
 */

#include "ELM.h"
#include "stdlib.h"
#include "VARS.h"


sElMeter*  mMeters;
uint8_t mNumOfMeters;


void ELM_Init(uint8_t numOfMeters)
{
	mNumOfMeters = numOfMeters;
	mMeters = malloc(numOfMeters * sizeof(sElMeter));
	if (mMeters == NULL)
	{
	  Error_Handler();
	}
}

void ELM_AddMeter(uint8_t ElmId, uint32_t pin, uint16_t VarId)
{
	if(ElmId < mNumOfMeters)
	{
		mMeters[ElmId].id = ElmId;
		mMeters[ElmId].Pin = pin;
		mMeters[ElmId].VarId = VarId;
		mMeters[ElmId].W = 0;
		mMeters[ElmId].Wh = 0;
		mMeters[ElmId].LastPulse = 0;
	}
}

void ELM_Update_1s(void)
{
	// copy the values to variables
	uint8_t i;
	uint32_t tick = HAL_GetTick();
	uint32_t period;
	uint16_t estimatedPower;
	for(i = 0 ; i < mNumOfMeters; i++)
	{
		period = tick - mMeters[i].LastPulse;
		if (period >  (3600000 / MIN_MEASURABLE_POWER_W))  // power decreased under measurable threshold
		{
			mMeters[i].W = 0;  // set power to zero
		}
		else if (period >  10000)  // after 10 seconds from last S0 pulse start decreasing measured power (worstcase estimation)
		{
			estimatedPower = 3600000 / period;  // actual estimated power
			if (mMeters[i].W > estimatedPower)
			{
				mMeters[i].W = estimatedPower;
			}
		}

		VAR_SetVariable(mMeters[i].VarId, mMeters[i].Wh, 1);
		VAR_SetVariable(mMeters[i].VarId + 10, mMeters[i].W, 1);
	}
}

uint16_t ELM_GetConsumptionWh(uint8_t ElmId)
{
	if(ElmId < mNumOfMeters)
	{
		return mMeters[ElmId].Wh;
	}
	else
	{
		return 0;
	}
}


uint16_t ELM_GetPowerW(uint8_t ElmId)
{
	if(ElmId < mNumOfMeters)
	{
		return mMeters[ElmId].W;
	}
	return 0;
}


void ELM_ResetConsumption(uint8_t ElmId)
{
	if(ElmId < mNumOfMeters)
	{
		mMeters[ElmId].Wh = 0;
	}
}


void ELM_ResetAllConsumptions(void)
{
	uint8_t i;
	for(i = 0 ; i < mNumOfMeters; i++)
	{
		mMeters[i].Wh = 0;
	}
}

// exti interrupt callback


void ELM_ExtiCallback(uint16_t GPIO_Pin)
{
	uint8_t i;
	uint32_t period;
	uint32_t tick = HAL_GetTick();
	for(i = 0 ; i < mNumOfMeters; i++)
	{
		if (mMeters[i].Pin == GPIO_Pin)
		{
			mMeters[i].Wh ++;
			period = tick - mMeters[i].LastPulse;
			if(period > 0)
			{
			  mMeters[i].W = 3600000/period;
			}
			mMeters[i].LastPulse = tick;
		}
	}
}
