/*
 * GEST.h
 *
 *  Created on: Dec 2, 2021
 *      Author: Martin
 *       Brief : Module for detection of special gestures on lightswitch buttons.
 *              All the button actions are handled by this module at first. If no gesture is detected, the button action is allowed to continue as standard
 *              light operation action processed by lightctrl.c
 *              Small delay is introduced to standard light control actions
 */

#ifndef INC_GEST_H_
#define INC_GEST_H_

#include "main.h"
//#include "app.h"


// gesture actions
#define GEST_AC5KW_TOGGLE					0
#define GEST_AC3KW_TOGGLE					1
#define GEST_ALL_LIGHTS_OFF					2
#define GEST_UPSTAIR_LIGHTS_OFF				3
#define GEST_DOWNSTAIR_LIGHTS_OFF			4
#define GEST_WSCRC_EAST_UP					5
#define GEST_WSCRC_EAST_DOWN				6
#define GEST_WSCRC_WEST_UP					7
#define GEST_WSCRC_WEST_DOWN				8

//#define GEST_STOVE_START            9
#define GEST_STOVE_AV_TOGGLE        10  // double press
#define GEST_STOVE_AV_AUTO          11  // tripple press
#define GEST_STOVE_CELANING         12  // long press


#define GEST_IRIG_DUAL            13
#define GEST_IRIG_TRIPLE          14

#define GEST_ACAXPERT_TOGGLE      15


#define MAX_NUM_OF_GESTURES  			20


#define SIM_PRESS_DURATION			100 //100ms


//#define EDI_DEFINED

typedef enum
{
  eDI_LO		= 0x00,
  eDI_HI		= 0x01
}eDI;


typedef enum
{
	egt_MultiButton,
	egt_MultiTouch
}eGestType;

typedef enum
{
	egs_Idle,
	egs_Pending,
	egs_Finishing
}eGestState;

typedef struct
{
	uint16_t id;
	uint16_t action;
	eGestType type;
	uint8_t num_of_touches;  // used only for multitouch type
	uint8_t num_of_buttons;
	uint8_t btnIds[4];    // max four buttons can be used in one gesture
	uint16_t timeout;
}sGestInit;


typedef struct
{
	sGestInit init;
	eGestState state;
	uint8_t touches;
	uint16_t timer;
	uint8_t triggerInput;
	eDI triggerState;
}sGesture;


typedef struct
{
  eGestState state;
  uint8_t triggerInput;
  eDI triggerState;
  uint8_t cnt;
  uint16_t timer;
}sGestInput;




void GEST_Init(void);

void GEST_DiInputChanged(uint8_t inputId, eDI state);

void GEST_AddGesture(sGestInit* gesture);

void GEST_Update_10ms(void);

#endif /* INC_GEST_H_ */
