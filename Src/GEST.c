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
#include "APP.h"
//#include "EXTDI.h"
#ifdef INC_EXTDI_H_
#include "EXTDI.h"
#endif

#ifdef _DI_H_
#include "DI.h"
#endif






sGesture* mGestures[MAX_NUM_OF_GESTURES];
//sGesture* mPendingGesture;
sGestInput mConsumedInput;
uint8_t mNumOfGestures;


void GEST_Init(void)
{
	uint8_t i;
	mConsumedInput.state = egs_Idle;
	mConsumedInput.cnt = 0;
	mNumOfGestures = 0;
	for (i = 0; i < MAX_NUM_OF_GESTURES; i++)
	{
		mGestures[i] = NULL;
	}
}


void GEST_DiInputChanged(uint8_t inputId, eDI state)
{

	uint8_t b,g;
	// if no gesture is pending
	if(mConsumedInput.state == egs_Idle)
	{
	   mConsumedInput.cnt = 0;
		// search if this button is used by some gesture
		if(state == eDI_HI)
		{
			for(g = 0; g < mNumOfGestures ; g++)
			{
				for(b = 0; b < mGestures[g]->init.num_of_buttons; b++)
				{
					if (mGestures[g]->init.btnIds[b] == inputId) // gesture detection starting
					{
					  mConsumedInput.state = egs_Pending;
					  mConsumedInput.triggerInput = inputId;
					  mConsumedInput.cnt ++;
						mGestures[g]->state = egs_Pending;
						mGestures[g]->timer = 0;
						mGestures[g]->triggerInput = inputId;   // store the button ID which activated gesture detection
						mGestures[g]->triggerState = state;     // kind of obsolete
						if (mGestures[g]->init.type == egt_MultiTouch)
						{
						   mGestures[g]->touches = 1;
						}
					}
				}
			}
		}

	}
	else
	{
		// for multitouch gestures. increment the counter, and reset timer

	   for(g = 0; g < mNumOfGestures ; g++)
	   {
	     if(mGestures[g]->state == egs_Pending)
	     {
	        if (mGestures[g]->init.type == egt_MultiTouch && state == eDI_HI)
            {
               for(b = 0; b < mGestures[g]->init.num_of_buttons; b++)
               {
                 if (mGestures[g]->init.btnIds[b] == inputId) // button assigned to pending gesture
                 {
                    mGestures[g]->touches ++;
                    mGestures[g]->timer = 0;
                 }
               }
            }
	      }

	   }

	}

//	if(mPendingGesture == NULL || state == eDI_LO)
	if(mConsumedInput.state == egs_Idle)
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
	uint8_t i,g;

	for(g = 0; g < mNumOfGestures ; g++)
	{
	  gestDetected = 0;

	  if (mGestures[g]->state == egs_Pending)  // gesture detection period pending
    {
 	    mGestures[g]->timer += 10;   // increment the timer
      switch (mGestures[g]->init.type)
      {
        case egt_MultiButton:
          gestDetected = 1;
          for (i = 0; i < mGestures[g]->init.num_of_buttons; i++)
          {
            if (DI_Get(mGestures[g]->init.btnIds[i]) == eDI_LO)  // check that all buttons are pressed
            {
              gestDetected = 0;  // if some is not pressed gesture is not detected
            }
          }
          break;
        case egt_MultiTouch:
          if (mGestures[g]->touches == mGestures[g]->init.num_of_touches)
          {
            if(mGestures[g]->timer >= mGestures[g]->init.timeout)  // if multitouch sequence was finished  (this allows to distinguish dual and triple pushes)
            {
              gestDetected = 1; // gesture is detected after num_of_touches touches
            }
          }
          break;
      }

      if (gestDetected)
      {
        APP_GestureDetected(mGestures[g]->init.action);  // gesture action
        mGestures[g]->state = egs_Idle;
        mConsumedInput.state = egs_Idle;
        // if gesture is detected, button action is NOT forwarded to light control
      }
      else
      {

        if(mGestures[g]->timer  >= mGestures[g]->init.timeout)
        {
          mGestures[g]->state = egs_Idle;
          mGestures[g]->timer = 0;
          if(mConsumedInput.cnt > 0)
          {
            mConsumedInput.cnt --;
          }
        }
      }
    }
	}


	// process the consumed inputs

	if (mConsumedInput.state == egs_Pending && mConsumedInput.cnt == 0)   // if all pending gestures which consumed the input are timed out
	{
	  APP_DiInputChanged(mConsumedInput.triggerInput, eDI_HI); // forward the rising edge
	  mConsumedInput.state = egs_Finishing;
	  mConsumedInput.timer = 0;
	}

	if (mConsumedInput.state == egs_Finishing)
  {
	  mConsumedInput.timer += 10;
    if(mConsumedInput.timer  >= SIM_PRESS_DURATION)
    {
      if (DI_Get(mConsumedInput.triggerInput) == eDI_LO) // .. if button was already released..
      {
        APP_DiInputChanged(mConsumedInput.triggerInput, eDI_LO); //.. simulate falling edge
      }
      mConsumedInput.state = egs_Idle;
    }
  }


}
