 /* LOG.h
 *
 *  Created on: 18.2.2025
 *      Author: Martin
 *       Brief: Simple logging functions
 */


#ifndef INC_LOG_H_
#define INC_LOG_H_


#include "main.h"


#define LOG_BCKP_FUNC_ID        5 //(RTC_BKP_DR5)
#define LOG_BCKP_FAULT_ADDR    6 //  (RTC_BKP_DR6)

// value written to LOG_BCKP_FUNC_ID before a self-reset; appears as STARTUP byte 3 (fault handlers use 10..13)
#define LOG_FUNC_ID_CAN_TX_STALL  30

// eleError payload byte 0 (error code)
#define LOG_ERR_CAN_TX_STALL      0x01  // bytes 1..5: event (1 recovery, 2 reset), TEC, REC, ESR[6:0] (LEC<<4|BOFF<<2|EPVF<<1|EWGF), recoveries (sat. 255)



typedef enum
{
  eleStartup,
  eleError,
  eleLog,
  eleStatus,
}eLogEvent;




void LOG_Startup(void);
void LOG_SaveToBackup(uint8_t reg, uint32_t val);

#endif
