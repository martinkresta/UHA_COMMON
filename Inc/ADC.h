/*
 * ADC.h
 *
 *  Created on: Jul 25, 2021
 *      Author: Martin
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_


#include "main.h"


#define ADC_VREF_MV					3313   // mV

void ADC_Init(ADC_HandleTypeDef*  hADC, DMA_HandleTypeDef* hDMA, uint8_t numOfChannels);

void ADC_StartConversion(void);
uint16_t ADC_GetValue(uint8_t channel);



#endif /* INC_ADC_H_ */
