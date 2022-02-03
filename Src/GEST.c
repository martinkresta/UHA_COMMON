/*
 * GEST.c
 *
 *  Created on: Dec 2, 2021
 *      Author: Martin
 *      Brief : Module for detection of special gestures on lightswitch buttons.
 *              All the button actions are handled by this module at first. If no gesture is detected, the button action is allowed to continue as standard
 *              light operation action processed by lightctrl.c
 *              Small delay is introduced to standard light control actions
 *
 */

#include "GEST.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


sGesture* mGestures[MAX_NUM_OF_GESTURES];
sGesture* mPendingGesture;
uint8_t mNumOfGestures;


void GEST_Init(void)
{
	uint8_t i;
	mPendingGesture = NULL;
	mNumOfGestures = 0;
	for (i = 0; i < MAX_NUM_OF_GESTURES; i++)
	{
		mGestures[i] == NULL;
	}
}


void GEST_DiInputChanged(uint8_t inputId, eDI state)
{

	uint8_t b,g;
	// if no gesture is pending
	if(mPendingGesture == NULL)
	{
		// search if this button is used by some gesture
		if(state == eDI_HI)
		{
			for(g = 0; g < mNumOfGestures ; g++)
			{
				for(b = 0; b < mGestures[g]->init.num_of_buttons; b++)
				{
					if (mGestures[g]->init.btnIds[b] == inputId) // gesture detection starting
					{
						mPendingGesture = mGestures[g];
						mPendingGesture->state = egs_Pending;
						mPendingGesture->timer = 0;
						mPendingGesture->triggerInput = inputId;   // store the button ID which activated gesture detection
						mPendingGesture->triggerState = state;     // kind of obsolete
						if (mPendingGesture->init.type == egt_MultiTouch)
						{
							mPendingGesture->touches = 1;
						}
					}
				}
			}
		}

	}
	else
	{
		// for multitouch gestures. increment the counter, and reset timer
		if (mPendingGesture->init.type == egt_MultiTouch && state == eDI_HI)
		{
			for(b = 0; b < mPendingGesture->init.num_of_buttons; b++)
			{
				if (mPendingGesture->init.btnIds[b] == inputId) // button assigned to pending gesture
				{
					mPendingGesture->touches ++;
					mPendingGesture->timer = 0;
				}
			}
		}
	}

//	if(mPendingGesture == NULL || state == eDI_LO)
	if(mPendingGesture == NULL)
	{
		APP_DiInputChanged(inputId, state);
	}
}

void GEST_AddGesture(sGestInit* gesture)
{
	mGestures[gesture->id] = malloc(sizeof(sGesture));
	if (mGestures[gesture->id] != NULL)
	{
	  memcpy(&(mGestures[gesture->id]->init),gesture, sizeof(sGestInit));
	  mNumOfGestures++;
	}
	else
	{
		// TBD malloc error
	}
}

void GEST_Update_10ms(void)
{
	uint8_t gestDetected = 0;
	uint8_t i;
	if (mPendingGesture != NULL)
	{
		if (mPendingGesture->state == egs_Pending)  // gesture detection period pending
		{
			switch (mPendingGesture->init.type)
			{
				case egt_MultiButton:
					gestDetected = 1;
					for (i = 0; i < mPendingGesture->init.num_of_buttons; i++)
					{
						if (DI_Get(mPendingGesture->init.btnIds[i]) == eDI_LO)  // check that all buttons are pressed
						{
							gestDetected = 0;  // if some is not pressed gesture is not detected
						}
					}
					break;
				case egt_MultiTouch:
					if (mPendingGesture->touches >= mPendingGesture->init.num_of_touches)
					{
						gestDetected = 1; // gesture is deteceted after num_of_touches touches
					}
					break;
			}
		}


		if (gestDetected)
		{
			APP_GestureDetected(mPendingGesture->init.action);  // gesture action
			mPendingGesture->state = egs_Idle;
			mPendingGesture = NULL;
			// if gesture is detected, button action is NOT forwarded to light control
		}
		else  // pass further as a standard button press
		{
			if  (mPendingGesture->state == egs_Pending)
			{
				mPendingGesture->timer += 10;
				if(mPendingGesture->timer  >= mPendingGesture->init.timeout)
				{
					mPendingGesture->state = egs_Finishing;
					mPendingGesture->timer = 0;
					APP_DiInputChanged(mPendingGesture->triggerInput, eDI_HI); // forward the rising edge
				}
			}
		}

		if (mPendingGesture->state == egs_Finishing)
		{
			mPendingGesture->timer += 10;
			if(mPendingGesture->timer  >= SIM_PRESS_DURATION)
			{
				if (DI_Get(mPendingGesture->triggerInput) == eDI_LO) // .. if button was already released..
				{
					APP_DiInputChanged(mPendingGesture->triggerInput, eDI_LO); //.. simulate falling edge
				}
				mPendingGesture->state = egs_Idle;
				mPendingGesture = NULL;
			}
		}
	}

}
