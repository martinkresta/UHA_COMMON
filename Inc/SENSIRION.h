/*
 * SENSIRION.h
 *
 *  Created on: 1. 10. 2022
 *      Author: marti
 */

#ifndef INC_SENSIRION_H_
#define INC_SENSIRION_H_



#include "main.h"
#include "uha_vars_gen.h"   /* eVarId */


#define SENS_MAX_SENSORS  10

#define SENS_I2C_ADR_SHT41    0x44
#define SENS_I2C_ADR_SCD41    0x62
#define SENS_I2C_ADR_SDP810   0x25

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF


typedef enum
{
  st_SHT4x,           // T  + RH
  st_SCD4x,           // CO2
  st_SPS30,           // dust
  st_SDP810_125       // diff pressure   125Pa
}e_SensType;



typedef struct
{
 uint8_t Id;
 e_SensType Type;
 I2C_HandleTypeDef* BusHandle;
 eVarId VarId_1;   /* VID_*: SHT4x temperature, SCD4x CO2, SDP810 differential pressure */
 eVarId VarId_2;   /* VID_*: SHT4x relative humidity */
}sI2cSensor;


void SENS_Init(void);

void SENS_AddSensor(sI2cSensor sensor);

int16_t SENS_ReadSensor(sI2cSensor* sensor);

void SENS_Update_1s(void);



#endif /* INC_SENSIRION_H_ */
