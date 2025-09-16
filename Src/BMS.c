/*
 * BMS.c
 *
 *  Created on: 7.2.2025
 *      Author: Martin
 *       Brief: Stateless Module for one-way communication with Smart BMS on battery pack ()
 */

#include "BMS.h"
#include "main.h"



static void DecodeData(uint8_t* data, sBMS* bms);
static uint8_t IsChecksumValid(uint8_t* data);
static void CalculateCellStats(sBMS* bms);


// Initialization of the BMS monitoring module
void BMS_Init(sBMS* bms)
{
  HAL_StatusTypeDef UartRetval;
  uint32_t UartError;
  if(bms->Huart != NULL)
  {
    bms->Com.PackInfoValid = 1;
    bms->Com.NewDataReady = 0;
    bms->Com.RecLength = 0;
    bms->Active = 0;

    UartRetval = HAL_UARTEx_ReceiveToIdle_DMA(bms->Huart, bms->Com.RxData, BMS_DMA_REC_LENGTH);
    if(UartRetval  != HAL_OK)
    {
      UartError = HAL_UART_GetError(bms->Huart);
    }
  }
}


// Update function, to the called periodically by the scheduler
void BMS_Update(sBMS* bms)
{
	HAL_StatusTypeDef UartRetval;
	uint32_t UartError;
	uint8_t* bmsData;

	bmsData = NULL; // Initialize pointer!!!

	if (bms->Com.NewDataReady)
	{
		if (bms->Com.RecLength >= (BMS_DATA_LENGTH - 1))
		{
			if (bms->Com.RecLength == 58)
			{
			  bmsData = &(bms->Com.RxData[0]);
			}
			else if (bms->Com.RecLength == 59)
			{
			  bmsData = &(bms->Com.RxData[1]);
			}

			if(bmsData != NULL)
			{
			  if (1 == IsChecksumValid(bmsData))
        {
          DecodeData(bmsData, bms);
          CalculateCellStats(bms);
          bms->Active = 1;
        }
        else
        {
          // TBD, report invalid checksum
        }
			}
		}
		else // incomplete message - ignore it
		{
			// TBD, report the thing
		}

		bms->Com.NewDataReady = 0;
	}
	UartRetval = HAL_UARTEx_ReceiveToIdle_DMA(bms->Huart, bms->Com.RxData, BMS_DMA_REC_LENGTH);
	if(UartRetval  != HAL_OK)
	{
		UartError = HAL_UART_GetError(bms->Huart);
	}
}

// Gets maximal cell voltage
uint16_t BMS_GetMaxCellVoltage(sBMS* bms)
{
	return bms->LiveData.MaxCellVoltage_mV;
}

// Gets minimal cell voltage
uint16_t BMS_GetMinCellVoltage(sBMS* bms)
{
  if (!bms->Active) return 3600;    //
	return bms->LiveData.MinCellVoltage_mV;
}

uint8_t BMS_IsChargingEnabled(sBMS* bms)
{
  if (!bms->Active) return 1;
	return bms->LiveData.Status & BMS_STAT_ALLOW_CHARGE;
}


uint8_t BMS_IsDischargingEnabled(sBMS* bms)
{
  if (!bms->Active) return 1;
  return bms->LiveData.Status & BMS_STAT_ALLOW_DISCHARGE;
}

// *********  Private functions ************
// checks the data in receive buffer, returns 1 if chksm is valid, 0 otherwise
static uint8_t IsChecksumValid(uint8_t* data)
{
	uint8_t i, sum;
	sum = 0;
	for (i = 0; i < (BMS_DATA_LENGTH - 1); i++)
	{
		sum += data[i];
	}
	if (data[BMS_DATA_LENGTH - 1] == sum)
	{
		return 1;  // checksum valid
	}
	else
	{
		return 0;  // checksum invalid
	}

}

static void DecodeData(uint8_t* data, sBMS* bms)
{
  bms->LiveData.VoltageTotal_mV = 5 * ((data[0]<<16)|(data[1]<<8)|(data[2]));
  if (data[4] != 'X')
  {
    bms->LiveData.ChargingCurrent_mA = (((data[4]<<8) | data[5])) * 125;
    if(data[4] == '-') bms->LiveData.ChargingCurrent_mA *= -1;
  }
  if (data[6] != 'X')
  {
    bms->LiveData.DischargingCurrent_mA = (((data[7]<<8) | data[8])) * 125;
    if(data[6] == '-') bms->LiveData.DischargingCurrent_mA *= -1;
  }
  if (data[9] != 'X')
  {
    bms->LiveData.BatteryCurrent_mA = (((data[10]<<8) | data[11])) * 125;
    if(data[9] == '-') bms->LiveData.BatteryCurrent_mA *= -1;
  }
  bms->LiveData.SOC = data[40];
  bms->LiveData.TodayCharging_Wh = (data[31]<<16)|(data[32]<<8)|(data[33]);
  bms->LiveData.TodayDischarging_Wh = (data[37]<<16)|(data[38]<<8)|(data[39]);
  bms->LiveData.Energystored_Wh = (data[34]<<16)|(data[35]<<8)|(data[36]);
  bms->LiveData.Status = data[30];
  bms->LiveData.TotalCharging_kWh = (data[41]<<16)|(data[42]<<8)|(data[43]);
  bms->LiveData.TotalDischarging_kWh = (data[44]<<16)|(data[45]<<8)|(data[46]);
  bms->Cells[(data[24] - 1)].Voltage_mV = 5 * ((data[26]<<8) | data[27]);
  bms->Cells[(data[24] - 1)].Temp_C= ((data[28]<<8) | data[29]) - 0x0114;

  bms->Info.NumOfCells =  data[25];
  bms->Info.Capacity_Wh = 100 * ((data[49]<<8) | data[50]);
  bms->Info.Vmin_mV = 5 * ((data[51]<<8) | data[52]);
  bms->Info.Vmax_mV = 5 * ((data[53]<<8) | data[54]);
  bms->Info.Vbalance_mV = 5 * ((data[55]<<8) | data[56]);

}

static void CalculateCellStats(sBMS* bms)
{
  bms->LiveData.MaxCellVoltage_mV = 0;
  bms->LiveData.MinCellVoltage_mV = bms->Cells[0].Voltage_mV ;
	uint8_t i;
	for (i = 0; i < 16; i++)
	{
		if (bms->Cells[i].Voltage_mV > bms->LiveData.MaxCellVoltage_mV)
		{
		  bms->LiveData.MaxCellVoltage_mV = bms->Cells[i].Voltage_mV;
		}

		if (bms->Cells[i].Voltage_mV < bms->LiveData.MinCellVoltage_mV)
		{
		  bms->LiveData.MinCellVoltage_mV = bms->Cells[i].Voltage_mV;
		}
	}
}

void BMS_UartRxCallback(UART_HandleTypeDef *huart, uint16_t reclength, sBMS* bms1, sBMS* bms2)
{

  if (huart->Instance == bms1->Huart->Instance)
  {
    bms1->Com.NewDataReady = 1;
    bms1->Com.RecLength = reclength;
  }
  if (huart->Instance  == bms2->Huart->Instance)
  {
    bms2->Com.NewDataReady = 1;
    bms2->Com.RecLength = reclength;
  }
}
