/**
  ******************************************************************************
  * File Name          : RTC.c
  * Description        : This file provides code for the configuration
  *                      of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "RTC.h"
#include <time.h>
#include <stdio.h>
#include "main.h"

/* USER CODE END 0 */


sDateTime RTC_GetTime()
{
  sDateTime dt;
  RTC_DateTypeDef sdatestructureget;
  RTC_TimeTypeDef stimestructureget;

  /* Get the RTC current Time */
  HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
  /* Get the RTC current Date */
  HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);

  dt.Year = 2000 + sdatestructureget.Year;
  dt.Month = sdatestructureget.Month;
  dt.Day = sdatestructureget.Date;
  dt.Hour = stimestructureget.Hours;
  dt.Minute = stimestructureget.Minutes;
  dt.Second = stimestructureget.Seconds;
  dt.WeekDay = sdatestructureget.WeekDay;
  return dt;
}

void RTC_SetTime(sDateTime dt)
{
  /* Configure RTC Calendar */
   RTC_DateTypeDef sdatestructure;
   RTC_TimeTypeDef stimestructure;

   /*##-1- Configure the Date #################################################*/
   sdatestructure.Year = dt.Year - 2000;
   sdatestructure.Month = dt.Month;
   sdatestructure.Date = dt.Day;
   sdatestructure.WeekDay = dt.WeekDay;

   if(HAL_RTC_SetDate(&hrtc,&sdatestructure,RTC_FORMAT_BIN) != HAL_OK)
   {
     /* Initialization Error */
   //  Error_Handler();
   }

   /*##-2- Configure the Time #################################################*/
   stimestructure.Hours = dt.Hour;
   stimestructure.Minutes = dt.Minute;
   stimestructure.Seconds = dt.Second;
   stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
   stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
   stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

   if(HAL_RTC_SetTime(&hrtc,&stimestructure,RTC_FORMAT_BIN) != HAL_OK)
   {
     /* Initialization Error */
   //  Error_Handler();
   }

   /*##-3- Writes a data in a RTC Backup data Register1 #######################*/
   HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
	
	// LOG_InsertGeneralMessage("Real Time Clock changed !");


 }

 
 void RTC_SetUnixTime(uint32_t unixtime)
 {
	 struct tm * tm_now;
	 time_t unixnow = unixtime;
	 tm_now = localtime(&unixnow);
	 sDateTime time;
	 time.Second = tm_now->tm_sec;
	 time.Minute = tm_now->tm_min;
	 time.Hour = tm_now->tm_hour;    
	 time.Day = tm_now->tm_mday;
	 time.Month = tm_now->tm_mon +1;
	 time.Year = tm_now->tm_year + 1900;
	 RTC_SetTime(time);
 }
 
 
 uint32_t RTC_GetUnixTime(void)
 {
	 struct tm tm_now;
	 sDateTime now = RTC_GetTime();
	 tm_now.tm_sec = now.Second;
	 tm_now.tm_min = now.Minute;
	 tm_now.tm_hour = now.Hour;
	 tm_now.tm_mday = now.Day;
	 tm_now.tm_mon = now.Month - 1;
	 tm_now.tm_year = now.Year - 1900;
	 return mktime(&tm_now);
 }
 
 // prints current time in format dd.mm.yyyy hh:mm:ss to string 
 // string with at least 19 characters must be passed
 void RTC_PrintTimeStamp(char* ts)
 {
	 sDateTime now = RTC_GetTime();
	 sprintf(ts,"%02d.%02d.%04d %02d:%02d:%02d", now.Day,now.Month,now.Year,now.Hour, now.Minute, now.Second);
	 return;
 }
 
  // prints current time in format dd.mm.yyyy hh:mm:ss.mmm to string 
 // string with at least 23 characters must be passed
 void RTC_PrintTimeStampMs(char* ts)
 {
	 uint16_t subseconds, ms;
	 subseconds = hrtc.Instance->SSR;
	 sDateTime now = RTC_GetTime(); 
	 ms = (1000 * ( RTC_PREDIV_S - subseconds))/(RTC_PREDIV_S - 1);   // see Reference manual, RTC Sub second register
	 sprintf(ts,"%02d.%02d.%04d %02d:%02d:%02d.%03d", now.Day,now.Month,now.Year,now.Hour, now.Minute, now.Second,ms);
	 return;
 }
 
  // prints current date in format ddmmyyyy to string 
 // string with at least 9 characters must be passed
 void RTC_PrintDate(char* ts)
 {
	 sDateTime now = RTC_GetTime();
	 sprintf(ts,"%02d%02d%04d", now.Day,now.Month,now.Year);
	 return;
 }


 // TBD : Prepare set of function for comparing datetime , time, time in period, etc.  IT IS COMPLEX!!

/* uint8_t RTC_IsNowBetweenDateTime(sDateTime from, sDateTime to)
 {
   sDateTime now = RTC_GetTime();
   uint32_t f = RTC_GetUnixTime(from);
   uint32_t t = RTC_GetUnixTime(to);
   uint32_t n = RTC_GetUnixTime(now);

   if(n >= f  && n <= t)
   {
     return 1;
   }
   else
   {
     return 0;
   }
 }


uint32_t RTC_GetSecondsFromMidnight


 void RTC_InitDateTime(sDatetime *dt)
 {
   dt->Year = 2000;
   dt->Month = 0;
   dt->Day = 0;
   dt->Hour = 0;
   dt->Minute = 0;
   dt->Second = 0;
 }*/


/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
