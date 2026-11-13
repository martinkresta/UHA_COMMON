/*
 * COM.h
 *
 *  Created on: Aug 14, 2021
 *      Author: Martin
 */

#ifndef INC_COM_H_
#define INC_COM_H_

#include "main.h"
#include "MCAN.h"
#include "LOG.h"
#include "APP_INFO.h"


// CAN ids
//#define  CMD_TM_DEV_ID  0x210
//#define  CMD_TM_STATUS  0x212
//#define  CMD_TM_VAR_VALUE  0x221


#define  CMD_BUTTON_STATE     0x100
#define  CMD_VAR_VALUE		    0x110   // old variable format: no longer sent; delete once no APP.c has its RX case (VARS2 step 6b part B)
#define  CMD_GESTURE			    0x120
#define  CMD_AC_REMOTE_REQ    0x130
#define  CMD_RECU_REMOTE_REQ  0x140



#define  CMD_BALANCE_INFO     0x310
#define  CMD_RTC_SYNC  		    0x510

#define  CMD_UHAMON_RECIEVE   0x600
#define  CMD_UHAMON_SEND      0x610

#define  CMD_LOG_MSG  0x620


#define  CMD_SYSINFO_REQ       0x550
#define  CMD_SYSINFO_RSP       0x560

#define  STREAM_LIST_SIZE		100




// redefinitions of some enums

typedef enum
{
  esit_ASW_version = 0,
  esit_UpTime,
  esit_HWID,
  // CAN diagnostics, answered by COM_SendSysInfoCommon() (UHA_COMMON), value = uint32 little endian
  esit_CanEsr = 0x10,      // live ESR: REC<<24 | TEC<<16 | LEC<<4 | BOFF<<2 | EPVF<<1 | EWGF
  esit_CanTsr,             // live TSR: TME2..0 at bits 28..26
  esit_CanTxRing,          // txRingMax<<16 | txRingLevel
  esit_CanTxDrops,         // txRingDropCnt
  esit_CanTxOk,            // txOkCnt
  esit_CanTxStall,         // stallFailCnt<<16 | stallRecoveryCnt (16 bit)
  esit_CanTxIdleMs,        // ms since last TXOK
  esit_CanErrStat,         // busOffCnt<<24 | errPassiveCnt<<16 (8 bit sat.) | recMax<<8 | tecMax
  esit_CanEsrAtStall,      // ESR captured at the last recovery
  esit_CanHalError,        // HAL_CAN_ERROR_* bits EWG..CRC seen since start
  esit_CanRepairs          // ptrRepairCnt<<24 | halStateRepairCnt<<16 | errLogCnt (16 bit)
}eSysInfoType;

typedef enum
{
  errm_AutoControl = 0,   // cancelling remote request
  errm_SligtOvepressure,
  errm_MaxOverpressure,
  errm_SlightUnderpressure,
  errm_MaxUnderpressure
}eRecuRemoteReqMode;


void COM_Init(uint8_t nodeId);

void COM_Update_10ms(void);

uint8_t COM_GetNodeStatus(uint8_t nodeId);
int16_t COM_GetNetworkStatus(void);
uint8_t COM_GetNodeId(void);

void COM_SendVariable(uint16_t varId);
void COM_SendMessage(uint16_t cobid, uint8_t* data, uint8_t dlc);

void COM_SendGesture(uint8_t gestureId);
void COM_SendACRemoteRequest(uint16_t type, uint16_t request, uint16_t keepOnTime);
void COM_SendRecuRemoteRequest(eRecuRemoteReqMode mode, uint16_t duration);
void COM_SendLogMsg(eLogEvent event, uint8_t* logvalue);  // logvalue may be array of bytes or  32bit number
void COM_SendSysInfo(eSysInfoType type, uint32_t value);
// answers the SysInfo types handled in UHA_COMMON (CAN diagnostics); returns 1 if handled, 0 otherwise.
// Call it from the default branch of the node's CMD_SYSINFO_REQ switch.
uint8_t COM_SendSysInfoCommon(uint8_t type);

// function  for sending UHAMON messages
void COM_SendUhamonMessage(uint8_t* uhamonMsg);

void COM_AddStreamedVariable(uint16_t varId, uint16_t period);

uint8_t COM_GetRxMessage( s_CanRxMsg* msg);




#endif /* INC_COM_H_ */
