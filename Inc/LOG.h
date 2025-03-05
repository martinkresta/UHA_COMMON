 /* LOG.h
 *
 *  Created on: 18.2.2025
 *      Author: Martin
 *       Brief: Simple logging functions
 */


#ifndef INC_LOG_H_
#define INC_LOG_H_


#include "main.h"


typedef enum
{
  eleStartup,
  eleError,
  eleLog,
  eleStatus,
}eLogEvent;



void LOG_Startup(void);
void LOG_SaveToBackup(uint32_t val);

#endif
