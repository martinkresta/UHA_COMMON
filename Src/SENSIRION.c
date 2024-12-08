/*
 * SENSIRION.c
 *
 *  Created on: 1. 10. 2022
 *      Author: marti
 */


#include "sensirion.h"
#include "VARS.h"
#include "main.h"


static sI2cSensor mSensors[SENS_MAX_SENSORS];
static uint8_t mNumOfSensors;
int16_t mDP;


int16_t Read_SHT4x(sI2cSensor* sens, int16_t* temperature, int16_t* humidity);

int16_t SCD4x_StartMeasurement(sI2cSensor* sens);
int16_t SCD4x_StopMeasurement(sI2cSensor* sens);
int16_t SCD4x_SetAltitude(sI2cSensor* sens, uint16_t alt);
int16_t SCD4x_Read(sI2cSensor* sens, int16_t* co2, int16_t* temperature, int16_t* humidity);


int16_t SDPx_StartMeasurement(sI2cSensor* sens);
int16_t SDPx_StopMeasurement(sI2cSensor* sens);
int16_t SDPx_Read(sI2cSensor* sens, int16_t* dp);

static uint8_t CalculateCRC(uint8_t* data, uint8_t len);


uint16_t scd_timer;

void SENS_Init(void)
{
  scd_timer = 0;
  mNumOfSensors = 0;
}

void SENS_AddSensor(sI2cSensor sensor)
{
 // assert(sensor.Id >= SENS_MAX_SENSORS);
  uint8_t i = sensor.Id;

  mSensors[mNumOfSensors] = sensor;

  // for some types of sensors it is necessary to start the measurements
  if(sensor.Type == st_SDP810_125)
  {
    HAL_Delay(5);
    SDPx_StartMeasurement(&sensor);
  }

  else if(sensor.Type == st_SCD4x)
  {
    HAL_Delay(5);
    SCD4x_SetAltitude(&sensor, 411);
    HAL_Delay(5);
    SCD4x_StartMeasurement(&sensor);
  }

  mNumOfSensors++;

}


int16_t SENS_ReadSensor(sI2cSensor* sens)
{
  switch(sens->Type)
  {
    case  st_SHT4x:
      //return Read_SHT4x(sens);
      break;
    case st_SCD4x:
      break;
    case st_SPS30:
      break;
    case st_SDP810_125:
      break;
  }
  return 0;
}



void SENS_Update_1s(void)
{
  int16_t co2,temp,hum,dp;

  uint8_t i, valid;



  for(i = 0; i < mNumOfSensors; i++)
  {
    hum = -1;
    temp = -1;
    co2 = -1;
    dp = -1;

    switch (mSensors[i].Type)
    {
      case  st_SHT4x:
       if(0 == Read_SHT4x(&(mSensors[i]),&temp, &hum)) valid = 1;
       else valid = 0;
       if (temp == -1 && hum == -1)  valid = 0;
       if(valid)
       {
         VAR_SetVariable(mSensors[i].VarId_1, temp, 1);
         VAR_SetVariable(mSensors[i].VarId_2, hum, 1);
       }
       else
       {
         VAR_SetVariable(mSensors[i].VarId_1, temp, 0);
         VAR_SetVariable(mSensors[i].VarId_2, hum, 0);
       }
       break;

     case st_SCD4x:
       if (scd_timer >= 6)
       {
         if(0 == SCD4x_Read(&(mSensors[i]),&co2, &temp, &hum)) valid = 1;
         else valid = 0;
         if (co2 == -1)  valid = 0;
         if(valid)
         {
           VAR_SetVariable(mSensors[i].VarId_1, co2, 1);
         }
         else
         {
           VAR_SetVariable(mSensors[i].VarId_1, co2, 0);
         }
       }
       break;

     case st_SPS30:
       // not supported
       break;

     case st_SDP810_125:
       if(0 == SDPx_Read(&(mSensors[i]), &dp)) valid = 1;
       else valid = 0;
      // if (dp == -1)  valid = 0;
       if(valid)
       {
         VAR_SetVariable(mSensors[i].VarId_1, dp, 1);
       }
       else
       {
         VAR_SetVariable(mSensors[i].VarId_1, dp, 0);
       }
       break;
    }
  }

  scd_timer++;
  if (scd_timer > 6) scd_timer = 0;
}



int16_t Read_SHT4x(sI2cSensor* sens, int16_t* temperature, int16_t* humidity)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  int16_t temp;
  int16_t rh;
  uint16_t temp_raw;
  uint16_t rh_raw;

  txData[0] = 0xFD;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, 0x88, txData, 1, 50))
  {
    return 1;
  }
  HAL_Delay(20);
  if(HAL_OK != HAL_I2C_Master_Receive(sens->BusHandle, 0x89, rxData, 6, 50))
  {
    return 1;
  }
  temp_raw = (uint16_t)rxData[0] * 256 + rxData[1];
  rh_raw = (uint16_t)rxData[3] * 256 + rxData[4];
  temp = -450 + (1750 * temp_raw)/0xFFFF;
  rh = -6 + (125 * rh_raw)/0xFFFF;
  if (rh > 100) rh = 100;
  if (rh < 0) rh = 0;
  *temperature = temp;
  *humidity = rh;
  return 0;
}


int16_t SCD4x_StartMeasurement(sI2cSensor* sens)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  txData[0] = 0x21;
  txData[1] = 0xB1;
  uint8_t wradd = (SENS_I2C_ADR_SCD41 << 1);
  uint8_t readd = (SENS_I2C_ADR_SCD41 << 1) | 0x01;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, wradd, txData, 2, 50))
  {
    return 1;
  }
}


int16_t SCD4x_StopMeasurement(sI2cSensor* sens)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  txData[0] = 0x3F;
  txData[1] = 0x86;
  uint8_t wradd = (SENS_I2C_ADR_SCD41 << 1);
  uint8_t readd = (SENS_I2C_ADR_SCD41 << 1) | 0x01;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, wradd, txData, 2, 50))
  {
    return 1;
  }
}


int16_t SCD4x_SetAltitude(sI2cSensor* sens, uint16_t alt)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  txData[0] = 0x24;
  txData[1] = 0x27;
  txData[2] = alt / 0xff;
  txData[3] = (alt & 0xFF);
  txData[4] = CalculateCRC(&(txData[2]),2);

  uint8_t wradd = (SENS_I2C_ADR_SCD41 << 1);
  uint8_t readd = (SENS_I2C_ADR_SCD41 << 1) | 0x01;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, wradd, txData, 5, 50))
  {
    return 1;
  }
}



int16_t SCD4x_Read(sI2cSensor* sens, int16_t* co2, int16_t* temperature, int16_t* humidity)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  int16_t temp;
  int16_t rh;
  int16_t co;

  uint16_t temp_raw;
  uint16_t rh_raw;
  uint16_t co2_raw;

  uint8_t wradd = (SENS_I2C_ADR_SCD41 << 1);
  uint8_t readd = (SENS_I2C_ADR_SCD41 << 1) | 0x01;

  txData[0] = 0xEC;
  txData[1] = 0x05;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, wradd, txData, 2, 50))
  {
    return 1;
  }
  HAL_Delay(5);
  if(HAL_OK != HAL_I2C_Master_Receive(sens->BusHandle, readd, rxData, 9, 100))
  {
    return 1;
  }
  co2_raw = (uint16_t)rxData[0] * 256 + rxData[1];
  temp_raw = (uint16_t)rxData[3] * 256 + rxData[4];
  rh_raw = (uint16_t)rxData[6] * 256 + rxData[7];
  co = co2_raw;
  temp = -45 + (175 * temp_raw)/0xFFFF;
  rh = (100 * rh_raw)/0xFFFF;
  *co2 = co;
  *temperature = temp;
  *humidity = rh;
  return 0;
}





int16_t SDPx_StartMeasurement(sI2cSensor* sens)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  txData[0] = 0x36;
  txData[1] = 0x15;
  uint8_t wradd = (SENS_I2C_ADR_SDP810 << 1);
  uint8_t readd = (SENS_I2C_ADR_SDP810 << 1) | 0x01;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, wradd, txData, 2, 50))
  {
    return 1;
  }
}


int16_t SDPx_StopMeasurement(sI2cSensor* sens)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  txData[0] = 0x3F;
  txData[1] = 0xF9;
  uint8_t wradd = (SENS_I2C_ADR_SDP810 << 1);
  uint8_t readd = (SENS_I2C_ADR_SDP810 << 1) | 0x01;

  if(HAL_OK != HAL_I2C_Master_Transmit(sens->BusHandle, wradd, txData, 2, 50))
  {
    return 1;
  }
}

int16_t SDPx_Read(sI2cSensor* sens, int16_t* dp)
{
  uint8_t rxData[10];
  uint8_t txData[10];

  int16_t pressure_raw;
  int16_t pressure;
  int16_t temp;

  uint8_t wradd = (SENS_I2C_ADR_SDP810 << 1);
  uint8_t readd = (SENS_I2C_ADR_SDP810 << 1) | 0x01;


  if(HAL_OK != HAL_I2C_Master_Receive(sens->BusHandle, readd, rxData, 9, 100))
  {
    return 1;
  }
  pressure_raw = (((int16_t)rxData[0] << 8) | rxData[1]);
  pressure = pressure_raw / 240;
  temp =  ((uint16_t)rxData[3] * 256 + rxData[4])/200;
  *dp = pressure;
  mDP = pressure;
  return 0;
}




static uint8_t CalculateCRC(uint8_t* data, uint8_t len)
{
  uint16_t current_byte;
  uint8_t crc = CRC8_INIT;
  uint8_t crc_bit;
  /* calculates 8-Bit checksum with given polynomial */
  for (current_byte = 0; current_byte < len; ++current_byte)
  {
    crc ^= (data[current_byte]);
    for (crc_bit = 8; crc_bit > 0; --crc_bit)
    {
      if (crc & 0x80)
      {
        crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      }
      else
      {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}
