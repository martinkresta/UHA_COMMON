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
  uint8_t scomLogData[10];
  uint32_t csr = RCC->CSR;  // read reset reason flags
  memset(logData,0,8);
  RCC->CSR |= RCC_CSR_RMVF;  // Reset reset reason flags
  logData[0] = THIS_NODE;
  logData[1] = eleStartup;
  logData[2] = (uint8_t)(csr >> 24);  // reset reason flags
  logData[3] = (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);  // saved value (Error Handlers)
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, 0);  // reset saved value;
#ifndef LOG_MASTER_NODE
  COM_SendLogMsg(eleStartup, &(logData[2]));
#else
  scomLogData[0] = CMD_LOG_MSG >> 8;
  scomLogData[1] = CMD_LOG_MSG & 0xFF;
  memcpy(&(scomLogData[2]), logData,8);
  SCOM_SendMsg(scomLogData,10);
#endif
}


void LOG_SaveToBackup(uint32_t val)
{
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, val);
}
