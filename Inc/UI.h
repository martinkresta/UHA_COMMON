/*
 * UI.h
 *
 *  Created on: Aug 15, 2021
 *      Author: Martin
 *       *      Brief: Control of on-board user interface (1x life led, 1x RGB led, 1x Buzzer)
 */
	
#ifndef UI_H
#define UI_H

#include "main.h"

typedef enum
{
	eUIL_NEGATIVE		= 0x00, 		/**< If is set negative -> <b> Pin in logic 0 means ACTIVE and pin in logic 1 mean INACTIVE</b> */
	eUIL_POSITIVE								/**< If is set positive -> <b> Pin in logic 0 means INACTIVE and pin in logic 1 mean ACTIVE</b> */
}eUIL;

/** 
  * @brief  LED status Enum
  */ 
typedef enum
{
	eUI_OFF						= 0x00,
	eUI_ON,
	eUI_BLINKING_SLOW,
	eUI_BLINKING_FAST,
	eUI_BLINK_ONCE,
	eUI_BEEP_ONCE,
	eUI_FLASH,
	eUI_COMBI_1,
	eUI_TOGGLE
}eUI_Status;

/** 
  * @brief LED control structure 
  */
typedef struct
{
	uint16_t				Led_Cnt;
	eUI_Status			Status;
	GPIO_TypeDef*		Port;
	uint32_t 				Pin;
	eUIL						Logic;
}sUI;

typedef struct
{
	GPIO_TypeDef*		Port;
	uint32_t 				Pin;
	eUIL						Logic;
}sUIpin;

typedef struct
{
	sUIpin Led_Life;
	sUIpin Led_R;
	sUIpin Led_G;
	sUIpin Led_B;
	sUIpin Buzzer;
}sUIHwInit;



void UI_Init (sUIHwInit* hw);
void UI_Update_10ms(void);
void UI_LED_R_SetMode(eUI_Status status);
void UI_LED_G_SetMode(eUI_Status status);
void UI_LED_B_SetMode(eUI_Status status);
void UI_LED_Life_SetMode(eUI_Status status);
void UI_Buzzer_SetMode(eUI_Status status);

#endif // LEDS_H
