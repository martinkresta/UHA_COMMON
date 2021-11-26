/*
 * SCOM.c
 *
 *  Created on: Aug 14, 2021
 *      Author: Martin
 *      Brief:  Universal CAN communication protocol between MCAN devices.
 *
 */

#include "RTC.h"
#include "main.h"
#include "COM.h"
#include "VARS.h"
#include "MCAN.h"


typedef struct
{
	uint8_t enable;
	uint16_t varId;
	uint16_t sendPeriod;  // 10ms
	uint16_t timer;
	void* next;
}sStreamVariable;



sStreamVariable mStreamList[STREAM_LIST_SIZE];
uint8_t mNodeId;


static void SendVariable(uint16_t id);


void COM_Init(uint8_t nodeId)
{
	mNodeId = nodeId;
}

uint8_t COM_GetNodeStatus(uint8_t nodeId)
{
	if (MCAN_GetNodesPt()[nodeId].canStatus == eNS_NMT_RUN)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int16_t COM_GetNetworkStatus(void)
{
	int16_t ns = 0;
	int16_t i;
	for (i = 0; i < MAX_NUM_OF_NODES; i++)
	{
		if (COM_GetNodeStatus(i) == 1)
		{
			ns |= (1 << i);
		}
	}
	return ns;
}

// just wrapper to MCAN
uint8_t COM_GetRxMessage( s_CanRxMsg* msg)
{
	return MCAN_GetRxMessage(msg);
}

void COM_Update_10ms(void)
{

	// stream the variables to CAN
	int i;
	for(i = 0; i < STREAM_LIST_SIZE; i++)
	{
		mStreamList[i].timer+=10;
	}
	for(i = 0; i < STREAM_LIST_SIZE; i++)
	{
		if (mStreamList[i].enable == 1 && mStreamList[i].sendPeriod != 0)
		{
			if (mStreamList[i].timer >= mStreamList[i].sendPeriod)
			{
				SendVariable(mStreamList[i].varId);
				mStreamList[i].timer = 0;
				break;  // send just 1 value every 10ms
			}
		}
	}
}


void COM_SendVariable(uint16_t varId)
{
	SendVariable(varId);
}

void COM_SendMessage(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	MCAN_SendFrame(cobid + mNodeId, data, dlc);
}


void COM_AddStreamedVariable(uint16_t varId, uint16_t period)
{
	// go thru the list to find if entry already exists
	int i;
	for(i = 0; i < STREAM_LIST_SIZE; i++)
	{
		if(mStreamList[i].varId == varId)
		{
			if(period != 0)
			{
				mStreamList[i].sendPeriod = period;
				mStreamList[i].enable = 1;
				return;
			}
			else
			{
				mStreamList[i].sendPeriod = 0;
				mStreamList[i].enable = 0;
			}
		}
	}

	// if not add variable to the list
	for(i = 0; i < STREAM_LIST_SIZE; i++)
	{
		if(mStreamList[i].enable == 0)
		{
			if(period != 0)
			{
				mStreamList[i].varId = varId;
				mStreamList[i].sendPeriod = period;
				mStreamList[i].enable = 1;
				return;
			}
		}
	}
}

/*Private methods*/
static void SendVariable(uint16_t id)
{
	uint16_t invalid = 0;
	uint16_t validflag = 0;
	int16_t tmp = VAR_GetVariable(id, &invalid);
	validflag = (invalid == INVALID_FLAG ? 0 : 1);
	uint8_t data[8];
	data[0] = id >> 8;
	data[1] = id  & 0xFF;
	data[2] = tmp >> 8;
	data[3] = tmp & 0xFF;
	data[4] = validflag >> 8;
	data[5] = validflag & 0xFF;
	data[6] = 0;
	data[7] = 0;
	MCAN_SendFrame(CMD_VAR_VALUE + mNodeId, data, 8);
}


