/*
 * UI.c
 *
 *  Created on: Aug 15, 2021
 *      Author: Martin
 *       *      Brief: Control of on-board user interface (1x life led, 1x RGB led, 1x Buzzer)
 */

/*************************
	I N C L U D E
*************************/
#include "UI.h"
#include "APP.h"

/*************************
	V A R I A B L E 
*************************/
sUI 	LED_Life;
sUI	  LED_R;
sUI	  LED_G;
sUI	  LED_B;
sUI   Buzzer;



/*************************
	F U N C T I O N
*************************/


static void UI_Control (sUI *led, uint8_t led_switch);
void UI_Struct_Initialisation (sUI init_struct, sUI *target);



void UI_Init (sUIHwInit* hw)
{
	sUI init_struct;
	
	init_struct.Status			= eUI_OFF;
	init_struct.Led_Cnt			= 0x00;
	
	init_struct.Pin					= hw->Led_Life.Pin;
	init_struct.Port				= hw->Led_Life.Port;
	init_struct.Logic				= hw->Led_Life.Logic;
	UI_Struct_Initialisation (init_struct, &LED_Life);
	
	init_struct.Pin					= hw->Led_R.Pin;
	init_struct.Port				= hw->Led_R.Port;
	init_struct.Logic				= hw->Led_R.Logic;
	UI_Struct_Initialisation (init_struct, &LED_R);

//#ifndef LIMITED_UI

	init_struct.Pin					= hw->Led_G.Pin;
	init_struct.Port				= hw->Led_G.Port;
	init_struct.Logic				= hw->Led_G.Logic;
	UI_Struct_Initialisation (init_struct, &LED_G);

#ifndef LIMITED_UI_LRG

	init_struct.Pin					= hw->Led_B.Pin;
	init_struct.Port				= hw->Led_B.Port;
	init_struct.Logic				= hw->Led_B.Logic;
	UI_Struct_Initialisation (init_struct, &LED_B);

	init_struct.Pin					= hw->Buzzer.Pin;
	init_struct.Port				= hw->Buzzer.Port;
	init_struct.Logic				= hw->Buzzer.Logic;
  UI_Struct_Initialisation (init_struct, &Buzzer);
#endif
}

/**
* @brief  Reading of actual states on defined Digital input
* @param	*di_input	Pointer to Digital input
*/
void UI_Struct_Initialisation (sUI init_struct, sUI *target)
{
	*target = init_struct;
	UI_Control(target,0);
}


// periodicaly scheduled updtae function
void UI_Update_10ms()
{
	UI_Control(&LED_Life, 	0x00);
	UI_Control(&LED_R, 	0x00);
//#ifndef LIMITED_UI
	UI_Control(&LED_G, 	0x00);
#ifndef LIMITED_UI_LRG
	UI_Control(&LED_B, 	0x00);
	UI_Control(&Buzzer, 	0x00);
#endif
}


void UI_LED_R_SetMode(eUI_Status status)
{
	if (LED_R.Status != status)
	{
		LED_R.Status = status;
		LED_R.Led_Cnt = 0;
	}
}

void UI_LED_G_SetMode(eUI_Status status)
{
	if (LED_G.Status != status)
	{
		LED_G.Status = status;
		LED_G.Led_Cnt = 0;
	}
}

void UI_LED_B_SetMode(eUI_Status status)
{
	if (LED_B.Status != status)
	{
		LED_B.Status = status;
		LED_B.Led_Cnt = 0;
	}
}

void UI_LED_Life_SetMode(eUI_Status status)
{
	if (LED_Life.Status != status)
	{
		LED_Life.Status = status;
		LED_Life.Led_Cnt = 0;
	}
}

void UI_Buzzer_SetMode(eUI_Status status)
{
	if (Buzzer.Status != status)
	{
		Buzzer.Status = status;
		Buzzer.Led_Cnt = 0;
	}
}

/**
  * @brief  Controlling states of LED 
  * @param  led: Pointer to specific LED
  * @param  led_switch: If isn't this parameter null, the function inverts status of specific LED, but LED has to be in TOGGLE mode!
  */
static void UI_Control (sUI *led,uint8_t led_switch)
{

	switch(led->Status)
	{
		case eUI_OFF:
			if(led->Logic == eUIL_NEGATIVE)
			{
				HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
			}
			else
			{
				HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
			}
			break;
		case eUI_ON:
			if(led->Logic == eUIL_NEGATIVE)
			{
				HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
			}
			else
			{
				HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
			}			
			break;
		case eUI_BLINKING_SLOW:
			if(led->Led_Cnt <= 50)
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}	
			}
			else
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
			}

			if(led->Led_Cnt >= 100)
			{
				led->Led_Cnt = 0;
			}
			break;
		case eUI_BLINKING_FAST:
			if(led->Led_Cnt <= 10 )
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}	
			}
			else
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
			}

			if(led->Led_Cnt >= 20)
			{
				led->Led_Cnt = 0;
			}			
			break;
		case eUI_BLINK_ONCE:
			if(led->Led_Cnt <= 5)  // blink for 50ms
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
			}				
			else
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				
				led->Status = eUI_OFF;   // go to permanent off state
				led->Led_Cnt = 0;
			}		
			break;
		 case eUI_BEEP_ONCE:
					if(led->Led_Cnt <= 300)  // beep for 3 sec
					{
						if(led->Logic == eUIL_NEGATIVE)
						{
							HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
						}
						else
						{
							HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
						}
					}
					else
					{
						if(led->Logic == eUIL_NEGATIVE)
						{
							HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
						}
						else
						{
							HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
						}

						led->Status = eUI_OFF;   // go to permanent off state
						led->Led_Cnt = 0;
					}
					break;
		case eUI_FLASH:
			if(led->Led_Cnt < 5)
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}	
			}
			else
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
			}

			if(led->Led_Cnt >= 100)
			{
				led->Led_Cnt = 0;
			}			
			break;
		case eUI_TOGGLE:
			if(led_switch)
			{
				HAL_GPIO_TogglePin(led->Port,led->Pin);
			}
			break;
		case eUI_COMBI_1:
			if(led->Led_Cnt < 10)
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}	
			}
			else if((led->Led_Cnt >= 10)&&(led->Led_Cnt < 50))
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
			}
			else if((led->Led_Cnt >= 50)&&(led->Led_Cnt < 60))
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}	
			}
			else if((led->Led_Cnt >= 60)&&(led->Led_Cnt < 100))
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
			}
			else if((led->Led_Cnt >= 100)&&(led->Led_Cnt < 150))
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}	
			}
			else if((led->Led_Cnt >= 150)&&(led->Led_Cnt < 200))
			{
				if(led->Logic == eUIL_NEGATIVE)
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_SET);
				}
				else
				{
					HAL_GPIO_WritePin(led->Port,led->Pin,GPIO_PIN_RESET);
				}
			}

			if(led->Led_Cnt >= 200)
			{
				led->Led_Cnt = 0;
			}			
			
			break;
		default:
			break;
	}
	
	if(led->Led_Cnt < 0xFFFF)
	{
		led->Led_Cnt++;		
	}	
}
