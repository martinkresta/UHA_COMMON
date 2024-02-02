/*
 * PID.h
 *  Universal implemetation of PID controller
 *  Created on: Dec  2023
 *      Author: Martin
 *  Brief:
 *
 *  How to Use:
 *
 */

#ifndef INC_PID_H_
#define INC_PID_H_

#include "main.h"

typedef struct
{
  uint16_t period_s;
  float error;
  float lastError;
  float pFactor;
  float iFactor;
  float dFactor;
  float pTerm;
  float iTerm;
  float dTerm;
  float rawAction;   // action before clamping
  float action;
  float minAction;
  float maxAction;
}tPid;



void PID_Init(tPid *pid);

float PID_Update(tPid *pid, float error);


#endif /* INC_PID_H_ */
