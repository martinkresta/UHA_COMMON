// Module: Temperature measurement module
// Target: STM32F4xx family, utilizing STM CUBE platform
// Author: Martin Kresta
// Date: 05.07.2016

// Brief: The module implements 1-Wire interface to read
// temperatures from multiple sensors DS18B20

/* Dependencies */
#include "main.h"

/* Macros */




#define MAX_ERR_TO_INVALIDATE		3  // max number of consecutive err readouts to set value as invalid



// temp sensors aliases (see TempSensIDs.xlsx)
#define      T1            0
#define      T2            1
#define      T3            2
#define      T4            3
#define      T5            4
#define      T6            5
#define      T7            6
#define      T8            7
#define      T9            8
#define      T10           9
#define      T101          10
#define      T102          11
#define      T103          12
#define      T104          13
#define      T105          14
#define      T106          15
#define      T107          16
#define      T108          17
#define      T109          18
#define      T110          19
#define      T301          20
#define      T302          21
#define      T303          22
#define      T304          23
#define      T305          24
#define      T306          25
#define      T307          26
#define      T308          27
#define      T309          28
#define      T310          29
#define      T_IOBOARD_U   30
#define      T_TECHM       31
#define      T_IOBOARD_D   32
#define      T_ELECON      33

#define NUM_OF_ALL_SENSORS 34



typedef enum
{
	ets_Valid,
	ets_NotReady,
	ets_NotValid,
	ets_NotConnected,
	ets_NotAssigned,
}eTempStatus;


typedef struct
{
	uint8_t sensorId;
	int16_t tempC_10ths;
	int16_t rawTempC_10ths;
	eTempStatus status;
	uint16_t VarId;
	uint8_t owBusId;
	uint8_t errorCnt;
}sTemp;

void TEMP_Init(void);
void TEMP_Update100ms(void);
uint8_t TEMP_AddHwBus(uint8_t busId, GPIO_TypeDef* port, uint32_t pin);
uint8_t TEMP_AssignSensor(uint8_t sensorId, uint8_t varId, uint8_t busId);


