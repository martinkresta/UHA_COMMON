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
  float newIterm;     //  Temporary I term value - needed for antiwindup implementation
  pid->lastError = error;


  pid->error = error;
  pid->pTerm = -error * pid->pFactor;
  pid->iTerm = pid->iTerm - error*pid->iFactor * pid->period_s;
  //newIterm = pid->iTerm - error*pid->iFactor * pid->period_s;
  pid->dTerm = -error_dt * pid->dFactor;
  pid->rawAction = pid->pTerm + pid->iTerm + pid->dTerm;

  if (pid->rawAction > pid->maxAction)
  {
    pid->action = pid->maxAction;
    pid->iTerm = pid->maxAction - pid->pTerm - pid->dTerm;  // anti windup
  }

  else if (pid->rawAction < pid->minAction)
  {
    pid->action = pid->minAction;
    pid->iTerm = pid->minAction - pid->pTerm - pid->dTerm;  // anti windup
  }
  else
  {
    pid->action = pid->rawAction;
   // pid->iTerm = newIterm;
  }

  return pid->action;

}
