/*
 * METEO.c
 *
 *  Created on: Aug 22, 2022
 *      Author: Martin
 *      Brief: Module for collecting outdoor meteo information (Wind speed sensor, Rainfall sensor)
 */

#include "METEO.h"
#include "stdlib.h"
#include "VARS2.h"


sWindSensor mWind;
sRainSensor mRain;
static sVarsMeteo* mpVars;   /* produced group Vars2.Meteo */




void METEO_Init(uint32_t windPin, uint32_t rainPin, sVarsMeteo* pVars)
{
   mpVars = pVars;
   mWind.Pin = windPin;
   mRain.Pin = rainPin;
   mWind.avgIdx = 0;
}


void METEO_Update_1s(void)
{
	uint8_t i;
	uint16_t sum;
	uint32_t tick = HAL_GetTick();
	uint32_t period;
	uint64_t thirdPower;
	uint64_t powerOfWind;

   period = tick - mWind.LastPulse;
   if (period >  MAX_WINDPULSE_PERIOD_MS)  // wind decreased under measurable threshold
   {
      mWind.WindSpeedBurst = 0;  // set wind to zero
   }

   mWind.buff[mWind.avgIdx++] = mWind.WindSpeedBurst;
   if(mWind.avgIdx >= WIND_AVG_WINDOW_S) mWind.avgIdx = 0;

   sum = 0;
   for(i = 0; i < WIND_AVG_WINDOW_S; i++)
   {
      sum += mWind.buff[i];
   }
   mWind.WindSpeedAvg = sum/WIND_AVG_WINDOW_S;

   // calculate theoretical power of wind on 1m2 Savonius turbine


   thirdPower = ((uint64_t)(mWind.WindSpeedAvg) * (uint64_t)(mWind.WindSpeedAvg) * (uint64_t)(mWind.WindSpeedAvg));
   powerOfWind = (5 * 1 * AIR_DENSITY_GM3 * thirdPower) / 10000000;
   mWind.PowerW = (uint16_t) (powerOfWind * TURBINE_EFFICIENCY_PCT) / 100;
   mWind.EnergyuWh += (uint64_t) (((uint64_t)mWind.PowerW * 1000000 )/ 3600);



   // publish (wind speeds are measured in dm/s)
   mpVars->WindBurst_mps.value = (float)mWind.WindSpeedBurst / 10.0f;
   mpVars->WindAvg_mps.value = (float)mWind.WindSpeedAvg / 10.0f;
   mpVars->WindPower_W.value = (float)mWind.PowerW;
   mpVars->WindEnergy_Wh.value = (float)(mWind.EnergyuWh / 1000000);
   VARS2_SetGroupValid(mpVars, 1);

}


uint16_t METEO_GetWindSpeedAvg(void)
{
   return mWind.WindSpeedAvg;
}

uint16_t METEO_GetWindSpeedBurst(void)
{
   return mWind.WindSpeedBurst;
}

uint16_t METEO_GetWindEnergyWh(void)
{
   return mWind.EnergyuWh/1000000;
}

void METEO_ResetWindEnergy(void)
{
   mWind.EnergyuWh = 0;
}

void METEO_ResetRainFall(void)
{
   mRain.CumulatedRainFall = 0;
}

// exti interrupt callback


void METEO_ExtiCallback(uint16_t GPIO_Pin)
{
	uint8_t i;
	uint32_t period;
	uint32_t tick = HAL_GetTick();

   if (mWind.Pin == GPIO_Pin)
   {
      period = tick - mWind.LastPulse;
      mWind.WindSpeedBurst = 6600/period;  // Sensor constant is 1 pulse per second = 2,375Kmh. We calculate in decimeter per second
      mWind.LastPulse = tick;
   }
}
