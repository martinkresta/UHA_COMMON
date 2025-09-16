 /* BMS.h
 *
 *  Created on: 7.2.2025
 *      Author: Martin
 *       Brief: Stateless Module for one-way communication with Smart BMS on battery pack ()
 */


#ifndef INC_BMS_H_
#define INC_BMS_H_


#include "main.h"
#include "RTC.h"

#define BMS_DATA_LENGTH 58   // documantation says 58,
#define BMS_DMA_REC_LENGTH 59 // maximal allowed length of DMA transfer


#define BMS_STAT_ALLOW_CHARGE					0x01
#define BMS_STAT_ALLOW_DISCHARGE			0x02
#define BMS_STAT_COM_ERROR						0x04
#define BMS_STAT_CELL_LOW_VOLTAGE			0x08
#define BMS_STAT_CELL_HIGH_VOLTAGE		0x10
#define BMS_STAT_LOW_TEMPERATURE			0x20
#define BMS_STAT_HIGH_TEMPERATURE			0x40
#define BMS_STAT_SOC_UNKNOWN					0x80

typedef struct{
	uint16_t Voltage_mV;
	int16_t Temp_C;
}sCell;

typedef struct{
	uint32_t Capacity_Wh;
	uint32_t Vmin_mV;
	uint32_t Vmax_mV;
	uint32_t Vbalance_mV;
	uint8_t NumOfCells;
}sBmsPackInfo;

typedef struct{
	uint32_t VoltageTotal_mV;
	int32_t ChargingCurrent_mA;
	int32_t DischargingCurrent_mA;
	int32_t BatteryCurrent_mA;
	uint32_t TodayCharging_Wh;
	uint32_t TodayDischarging_Wh;
	uint32_t Energystored_Wh;
	uint32_t TotalCharging_kWh;
	uint32_t TotalDischarging_kWh;
	uint8_t Status;
	uint8_t SOC;
	uint16_t MaxCellVoltage_mV;
	uint16_t MinCellVoltage_mV;
	//sCell* Cells;   // dynamically allocated array of cell modules
}sLiveData;

typedef struct
{
  uint8_t RxData[BMS_DMA_REC_LENGTH +2];  // buffer for reception of raw BMS data
  uint8_t NewDataReady;
  uint8_t RecLength;
  uint8_t PackInfoValid;
}sComData;    // data used for nonblocking communication (only for internal purposes)

typedef struct{
  UART_HandleTypeDef* Huart;
  uint8_t Active;  // bool flag  indicating if communication is working
  sDateTime LastRefresh;
  sBmsPackInfo Info;
  sLiveData LiveData;
  sCell Cells[16];
  sComData Com;
}sBMS;



void BMS_Init(sBMS* bms);
// Update function, to the called periodically by the scheduler
void BMS_Update(sBMS* bms);
// Gets maximal cell voltage
uint16_t BMS_GetMaxCellVoltage(sBMS* bms);
// Gets minimal cell voltage
uint16_t BMS_GetMinCellVoltage(sBMS* bms);
// gets status of charge enable relay
uint8_t BMS_IsChargingEnabled(sBMS* bms);
uint8_t BMS_IsDischargingEnabled(sBMS* bms);


void BMS_UartRxCallback(UART_HandleTypeDef *huart, uint16_t reclength, sBMS* bms1, sBMS* bms2);



#endif
