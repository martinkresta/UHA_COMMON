/*
 * ADC.c
 *
 *  Created on: Jul 25, 2021
 *      Author: Martin
 */

#include "main.h"
#include "adc.h"
#include <stdlib.h>


ADC_HandleTypeDef* adc;
DMA_HandleTypeDef* dma;

uint8_t mNumOfChannels;
uint16_t* mResults = NULL;


void ADC_Init(ADC_HandleTypeDef*  hADC, DMA_HandleTypeDef* hDMA, uint8_t numOfChannels)
{
	adc = hADC;
	dma = hDMA;
	mNumOfChannels = numOfChannels;
#ifdef STM32L4    // calibration supported only by STM32L4xx family
	HAL_ADCEx_Calibration_Start(adc, ADC_SINGLE_ENDED);
#endif
	mResults = malloc(mNumOfChannels * sizeof(uint16_t));
}


void ADC_StartConversion(void)
{
	if (mResults != NULL)
	{
		HAL_ADC_Start_DMA(adc, (uint32_t*)mResults, mNumOfChannels);
	}
}

uint16_t ADC_GetValue(uint8_t channel)
{
	if (channel < mNumOfChannels && mResults != NULL)
	{
		return mResults[channel];
	}
	else
	{
		return 0;
	}
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	HAL_ADC_Stop_DMA(adc);
}
