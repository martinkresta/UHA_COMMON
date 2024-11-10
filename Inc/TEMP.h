// Module: Temperature measurement module
// Target: STM32F4xx family, utilizing STM CUBE platform
// Author: Martin Kresta
// Date: 05.07.2016

// Brief: The module implements 1-Wire interface to read
// temperatures from multiple sensors DS18B20

/* Dependencies */
#include "main.h"

/* Macros */




#define MAX_ERR_TO_INVALIDATE		5  // max number of consecutive err readouts to set value as invalid



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
#define      T10            9
#define      T101            10
#define      T102            11
#define      T103            12
#define      T104            13
#define      T105            14
#define      T106            15
#define      T107            16
#define      T108            17
#define      T109            18
#define      T110            19
#define      T111            20
#define      T112            21
#define      T113            22
#define      T114            23
#define      T115            24
#define      T116            25
#define      T301            26
#define      T302            27
#define      T303            28
#define      T304            29
#define      T305            30
#define      T306            31
#define      T307            32
#define      T308            33
#define      T309            34
#define      T310            35
#define      T_IOBOARD_U            36
#define      T_TECHM            37
#define      T_IOBOARD_D            38
#define      T_ELECON            39
#define      T_ELECON_D            40


#define NUM_OF_ALL_SENSORS 41



typedef enum
{
	ets_NotValid,
	ets_Valid,
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
uint8_t TEMP_AssignSensor(uint8_t sensorId, uint16_t varId, uint8_t busId);


