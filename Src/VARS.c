/*
 * VARS.c
 *
 *  Created on: Jul 13, 2021
 *      Author: Martin
 *
 *  Temporary compatibility layer over Vars2 (VARS2 direct access task, part A),
 *  removed in part C. The old Vars[] is gone; values and validity live in Vars2.
 */


#include "VARS.h"
#include "VARS2.h"


void VAR_Init(void)
{
}

void VAR_EnableVars2Read(void)
{
}

int16_t VAR_GetVariable(uint16_t varId, uint16_t* invalid)
{
	return VARS2_GetLegacy(varId, invalid, INVALID_FLAG);
}

void VAR_SetVariable(uint16_t varId, int16_t value, uint8_t valid)
{
	(void)VARS2_SetLegacy(varId, value, valid);
}
