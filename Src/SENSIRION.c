/*
 * SENSIRION.c
 *
 *  Created on: 1. 10. 2022
 *      Author: marti
 */


#include "sensirion.h"


static sI2cSensor mSensors[SENS_MAX_SENSORS];


int16_t Read_SHT4x(sI2cSensor* sens);


void SENS_Init(void)
{

}

void SENS_AddSensor(sI2cSensor sensor)
{
 // assert(sensor.Id >= SENS_MAX_SENSORS);
  uint8_t i = sensor.Id;

  mSensors[i] = sensor;

}

int16_t SENS_ReadSensor(sI2cSensor* sens)
{
  switch(sens->Type)
  {
    case  st_SHT4x:
      return Read_SHT4x(sens);
      break;
    case st_SCD4x:
      break;
    case st_SPS30:
      break;
    case st_SDP810_125:
      break;
  }
}



void SENS_Update_1s(void);


int16_t Read_SHT4x(sI2cSensor* sens)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  int16_t temp;
  int16_t rh;
  uint16_t temp_raw;
  uint16_t rh_raw;

  txData[0] = 0xFD;

  HAL_I2C_Master_Transmit(sens->BusHandle, 0x88, txData, 1, 1000);
  HAL_Delay(20);
  HAL_I2C_Master_Receive(sens->BusHandle, 0x89, rxData, 6, 1000);
  temp_raw = (uint16_t)rxData[0] * 256 + rxData[1];
  rh_raw = (uint16_t)rxData[3] * 256 + rxData[4];
  temp = -45 + (175 * temp_raw)/0xFFFF;
  rh = -6 + (125 * rh_raw)/0xFFFF;
  if (rh > 100) rh = 100;
  if (rh < 0) rh = 0;

  return rh;

}
