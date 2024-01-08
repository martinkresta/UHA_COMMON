/*
 * PID.c
 *  Universal implemetation of PID controller
 *  Created on: Dec  2023
 *      Author: Martin
 *  Brief:
 *
 *  How to Use:
 *
 */

#include "PID.h"

void PID_Init(tPid *pid)
{
  pid->iTerm = 0;
}


// calculates and return required control action
float PID_Update(tPid *pid, float error)
{
  float error_dt =  (error - pid->lastError)/pid->period_s;    // time derivation of error


  pid->error = error;
  pid->pTerm = -error * pid->pFactor;
  pid->iTerm = pid->iTerm - error*pid->iFactor * pid->period_s;
  pid->dTerm = -error_dt * pid->dFactor;
  pid->action = pid->pTerm + pid->iTerm + pid->dTerm;

  if (pid->action > pid->maxAction)
  {
    pid->action = pid->maxAction;
    pid->iTerm = pid->maxAction - pid->pTerm - pid->dTerm;  // anti windup
  }

  if (pid->action < pid->minAction)
  {
    pid->action = pid->minAction;
    pid->iTerm = pid->minAction + pid->pTerm + pid->dTerm;  // anti windup
  }

  return pid->action;

}
