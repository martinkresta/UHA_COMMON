 /* LOG.c
 *
 *  Created on: 18.2.2025
 *      Author: Martin
 *       Brief: Simple logging functions
 */

#include "LOG.h"
#include "main.h"
#include "RTC.h"
#include "APP.h"
#include "COM.h"
#include <string.h>

#ifdef LOG_MASTER_NODE
#include "SCOM.h"
#endif



void LOG_Startup(void)
{
  uint8_t logData[8];
#ifdef LOG_MASTER_NODE
  uint8_t scomLogData[10];
#endif
  uint32_t csr = RCC->CSR;  // read reset reason flags
  uint32_t fault_addr = 0;
  memset(logData,0,8);
  RCC->CSR |= RCC_CSR_RMVF;  // Reset reset reason flags
  logData[0] = THIS_NODE;
  logData[1] = eleStartup;
  logData[2] = (uint8_t)(csr >> 24);  // reset reason flags
  logData[3] = (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, LOG_BCKP_FUNC_ID);  // saved value (Error Handlers)
  fault_addr = HAL_RTCEx_BKUPRead(&hrtc, LOG_BCKP_FAULT_ADDR);
  memcpy(&(logData[4]),&fault_addr,4);
  HAL_RTCEx_BKUPWrite(&hrtc, LOG_BCKP_FUNC_ID, 0);  // reset saved value;
  HAL_RTCEx_BKUPWrite(&hrtc, LOG_BCKP_FAULT_ADDR, 0);  // reset saved value;
#ifndef LOG_MASTER_NODE
  COM_SendLogMsg(eleStartup, &(logData[2]));
#else
  scomLogData[0] = CMD_LOG_MSG >> 8;
  scomLogData[1] = CMD_LOG_MSG & 0xFF;
  memcpy(&(scomLogData[2]), logData,8);
  SCOM_SendMsg(scomLogData,10);
#endif
}


void LOG_SaveToBackup(uint8_t reg, uint32_t val)
{
  HAL_RTCEx_BKUPWrite(&hrtc, reg, val);
}


// Overrides the weak default in MCAN.c. Called in main context from MCAN_Update_10ms().
// payload: 6 bytes, byte 0 = MCAN_ERR_* code (see MCAN.h). Non-master nodes queue a 0x620 eleError frame into the
// TX ring (after a TX stall it goes out once CAN works again); the master node (ELECON) sends it over SCOM, which
// does not depend on CAN.
void MCAN_ReportError(const uint8_t payload[6], uint8_t beforeReset)
{
  uint8_t logData[8];

  logData[0] = THIS_NODE;
  logData[1] = eleError;
  memcpy(&(logData[2]), payload, 6);

  if (beforeReset)
  {
    LOG_SaveToBackup(LOG_BCKP_FUNC_ID, LOG_FUNC_ID_CAN_TX_STALL);
  }
#ifndef LOG_MASTER_NODE
  COM_SendLogMsg(eleError, &(logData[2]));
#else
  {
    uint8_t scomLogData[10];
    scomLogData[0] = CMD_LOG_MSG >> 8;
    scomLogData[1] = CMD_LOG_MSG & 0xFF;
    memcpy(&(scomLogData[2]), logData, 8);
    SCOM_SendMsg(scomLogData, 10);
  }
#endif
}
