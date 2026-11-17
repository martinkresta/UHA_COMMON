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
#include "VARS2.h"
#include "CMD.h"
#include "MCAN.h"
#include "string.h"


uint8_t mNodeId;


void COM_Init(uint8_t nodeId)
{
	mNodeId = nodeId;
	VARS2_Init(nodeId);
}


uint8_t COM_GetNodeId(void)
{
  return mNodeId;
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

// wrapper to MCAN; VARS2 frames (0x180+node) and command frames (0x200+node) are
// consumed here and never returned to the application - keep fetching until
// another frame or an empty buffer is found
uint8_t COM_GetRxMessage( s_CanRxMsg* msg)
{
	while (1 == MCAN_GetRxMessage(msg))
	{
		if ((msg->header.StdId & 0x7F0) == CMD_VARS2)
		{
			VARS2_ProcessRxFrame((uint16_t)msg->header.StdId, (uint8_t)msg->header.DLC,
					(msg->header.RTR != CAN_RTR_DATA), msg->data);
			continue;
		}
		// command frames 0x200 + target node (CMD_spec.md sec.4): the own ones go to CMD, the others are
		// dropped here, so the application never sees a command frame
		if ((msg->header.StdId & 0x7F0) == CMD_NODE_COMMAND)
		{
			if ((msg->header.StdId & 0x00F) == mNodeId && msg->header.DLC == 8 && msg->header.RTR == CAN_RTR_DATA)
			{
				(void)CMD_ProcessFrame(msg->data);
			}
			continue;
		}
		return 1;
	}
	return 0;
}

void COM_Update_10ms(void)
{
	// VARS2: this node's variables by their YAML period (1 CAN frame per 10 ms), receive timeouts
	VARS2_Update_10ms();
}


void COM_SendMessage(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	MCAN_SendFrame(cobid + mNodeId, data, dlc);
}




// wrapper for sending gesture command
void COM_SendGesture(uint8_t gestureId)
{
  uint8_t data[8];
  data[0] = gestureId >> 8;
  data[1] = gestureId & 0xFF;
  data[2] = 0;
  data[3] = 0;
  data[4] = 0;
  data[5] = 0;
  data[5] = 0;
  data[7] = 0;
  COM_SendMessage(CMD_GESTURE, data, 8);
}


// wrapper for sending AC remote request command
void COM_SendACRemoteRequest(uint16_t type, uint16_t request, uint16_t keepOnTime)
{
  uint8_t data[8];
  data[0] = type >> 8;
  data[1] = type & 0xFF;
  data[2] = request >> 8;
  data[3] = request & 0xFF;
  data[4] = keepOnTime >> 8;
  data[5] = keepOnTime & 0xFF;
  data[5] = 0;
  data[7] = 0;
  COM_SendMessage(CMD_AC_REMOTE_REQ, data, 8);
}

// wrapper for sending RECU remote request command
void COM_SendRecuRemoteRequest(eRecuRemoteReqMode mode, uint16_t duration)
{
  uint8_t data[8];
  data[0] = mode >> 8;
  data[1] = mode & 0xFF;
  data[2] = duration >> 8;
  data[3] = duration & 0xFF;
  data[4] = 0;
  data[5] = 0;
  data[5] = 0;
  data[7] = 0;
  COM_SendMessage(CMD_RECU_REMOTE_REQ, data, 8);
}


void COM_SendLogMsg(eLogEvent event, uint8_t* logvalue)  // logvalue may be array of up to 6 bytes
{
  uint8_t data[8];
  data[0] = mNodeId;
  data[1] = event;
  if(logvalue != NULL)
  {
    memcpy(&(data[2]), logvalue, 6);
  }
  COM_SendMessage(CMD_LOG_MSG, data, 8);
}


void COM_SendSysInfo(eSysInfoType type, uint32_t value)
{
  uint8_t data[8];
  memset (data, '0', 8);
  data[0] = mNodeId;
  data[1] = type;
  memcpy(&(data[2]), &(value), 4);

  COM_SendMessage(CMD_SYSINFO_RSP, data, 8);
}

static uint32_t Sat8(uint32_t v)
{
  return v > 255u ? 255u : v;
}

uint8_t COM_SendSysInfoCommon(uint8_t type)
{
  const s_McanDiag* d = MCAN_GetDiag();
  uint32_t v;

  switch (type)
  {
    case esit_CanEsr:        v = MCAN_GetEsr(); break;
    case esit_CanTsr:        v = MCAN_GetTsr(); break;
    case esit_CanTxRing:     v = ((uint32_t)d->txRingMax << 16) | d->txRingLevel; break;
    case esit_CanTxDrops:    v = d->txRingDropCnt; break;
    case esit_CanTxOk:       v = d->txOkCnt; break;
    case esit_CanTxStall:    v = ((uint32_t)d->stallFailCnt << 16) | (d->stallRecoveryCnt & 0xFFFFu); break;
    case esit_CanTxIdleMs:   v = HAL_GetTick() - d->lastTxOkTick; break;
    case esit_CanErrStat:    v = (Sat8(d->busOffCnt) << 24) | (Sat8(d->errPassiveCnt) << 16) | ((uint32_t)d->recMax << 8) | d->tecMax; break;
    case esit_CanEsrAtStall: v = d->esrAtStall; break;
    case esit_CanHalError:   v = d->halErrSeen; break;
    case esit_CanRepairs:    v = (Sat8(d->ptrRepairCnt) << 24) | (Sat8(d->halStateRepairCnt) << 16) | (d->errLogCnt & 0xFFFFu); break;
    default:                 return 0;
  }
  COM_SendSysInfo((eSysInfoType)type, v);
  return 1;
}




// function  for sending UHAMON messages
void COM_SendUhamonMessage(uint8_t* uhamonMsg)
{
  uint8_t data[8];
  memcpy(data, uhamonMsg, 8);
  data[0] = mNodeId;
  COM_SendMessage(CMD_UHAMON_SEND + mNodeId, data, 8);
}



