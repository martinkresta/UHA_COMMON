// Module: Temperature measurement module working above OW library
// Author: Martin Kresta
// Date: 23.22021

// Brief: The module contains the ROM codes of available temperature sensors.
// Aplication has to define witch sensors are connected, and should be measured

/* Dependencies */
#include <UI.h>
#include "TEMP.h"
#include "OW.h"
#include "main.h"
#include "VARS2.h"
#include "UI.h"
#include "LOG.h"
#include <stdlib.h>
#include <string.h>


/* Private variables */
uint8_t mNumOfAssignedSensors;
sTemp mSensors[NUM_OF_ALL_SENSORS];
uint16_t mTimer;
uint8_t mReadId;
uint8_t mConvertId;
uint8_t mNumOfBuses;

static int8_t mPendingIdx = -1;     // sensor whose readout was started in the last call (scratchpad not checked yet)
static uint16_t mIgnored85;         // ignored 85.0 degC readouts since start
static uint32_t mLast85LogTick;
static uint8_t mLogged85Once;


// temp sensors ROM codes (see TempSensIDs.xlsx)
uint8_t mSensorsAddress[NUM_OF_ALL_SENSORS][8] =   // LSB on the left, transmit LSB first!!
{
      {0x28,  0xFF,  0x2A,  0x19,  0xB2,  0x15, 0x04,  0x27},                      //T1
      {0x28,  0xFF,  0xC5,  0x6D,  0xB2,  0x15, 0x03,  0xB7},                      //T2
      {0x28,  0xFF,  0xD3,  0x03,  0xB2,  0x15, 0x01,  0x6E},                      //T3
      {0x28,  0xFF,  0x51,  0x74,  0xB2,  0x15, 0x03,  0xB4},                      //T4
      {0x28,  0xFF,  0x00,  0x94,  0xB2,  0x15, 0x03,  0xA5},                      //T5
      {0x28,  0xFF,  0xE4,  0x02,  0xB2,  0x15, 0x01,  0x34},                      //T6
      {0x28,  0xFF,  0x74,  0x2F,  0xB2,  0x15, 0x01,  0x6E},                      //T7
      {0x28,  0xFF,  0x5A,  0x9A,  0xB2,  0x15, 0x01,  0x24},                      //T8
      {0x28,  0xFF,  0xB5,  0x82,  0xB2,  0x15, 0x03,  0x09},                      //T9
      {0x28,  0xFF,  0x62,  0x1D,  0xB2,  0x15, 0x01,  0xC1},                      //T10
      {0x28,  0xC3,  0x04,  0x56,  0xB5,  0x01, 0x3C,  0x46},                      //T101
      {0x28,  0x76,  0xAE,  0x56,  0xB5,  0x01, 0x3C,  0x24},                      //T102
      {0x28,  0xB5,  0x1B,  0x56,  0xB5,  0x01, 0x3C,  0x7F},                      //T103
      {0x28,  0x0D,  0x88,  0x56,  0xB5,  0x01, 0x3C,  0x20},                      //T104
      {0x28,  0xBE,  0xDA,  0x56,  0xB5,  0x01, 0x3C,  0x68},                      //T105
      {0x28,  0x33,  0xCB,  0x56,  0xB5,  0x01, 0x3C,  0x79},                      //T106
      {0x28,  0xC2,  0x1B,  0x56,  0xB5,  0x01, 0x3C,  0x62},                      //T107
      {0x28,  0xF4,  0xA2,  0x56,  0xB5,  0x01, 0x3C,  0x81},                      //T108
      {0x28,  0xB2,  0x25,  0x56,  0xB5,  0x01, 0x3C,  0xDC},                      //T109
      {0x28,  0x15,  0xF8,  0x56,  0xB5,  0x01, 0x3C,  0xB7},                      //T110
      {0x28,  0xFF,  0x65,  0x76,  0xE0,  0x01, 0x3C,  0x34},                      //T111
      {0x28,  0xAC,  0x20,  0x76,  0xE0,  0x01, 0x3C,  0x78},                      //T112
      {0x28,  0x57,  0x65,  0x76,  0xE0,  0x01, 0x3C,  0xC9},                      //T113
      {0x28,  0x58,  0x73,  0x76,  0xE0,  0x01, 0x3C,  0x0D},                      //T114
      {0x28,  0x45,  0x06,  0x76,  0xE0,  0x01, 0x3C,  0xA3},                      //T115
      {0x28,  0xCA,  0x43,  0x76,  0xE0,  0x01, 0x3C,  0x56},                      //T116
      {0x28,  0x59,  0xCB,  0x46,  0x3A,  0x19, 0x01,  0xA9},                      //T301
      {0x28,  0x59,  0xB1,  0x56,  0xB5,  0x01, 0x3C,  0xA5},                      //T302
      {0x28,  0xF4,  0x55,  0x4D,  0x3A,  0x19, 0x01,  0x3E},                      //T303
      {0x28,  0x48,  0xD8,  0xBB,  0x1E,  0x19, 0x01,  0x11},                      //T304
      {0x28,  0x3C,  0x36,  0x67,  0x3A,  0x19, 0x01,  0x34},                      //T305
      {0x28,  0x5F,  0x74,  0x56,  0xB5,  0x01, 0x3C,  0xE7},                      //T306
      {0x28,  0x51,  0x37,  0x56,  0xB5,  0x01, 0x3C,  0x53},                      //T307
      {0x28,  0x62,  0x45,  0x56,  0xB5,  0x01, 0x3C,  0x09},                      //T308
      {0x28,  0xE4,  0x27,  0x7B,  0x3A,  0x19, 0x01,  0xCA},                      //T309
      {0x28,  0x1E,  0x36,  0x02,  0x3A,  0x19, 0x01,  0xFD},                      //T310
      {0x28,  0x4C,  0xA3,  0x7E,  0x0C,  0x0, 0x0,  0x6D},                      //T_IOBOARD_U
      {0x28,  0x60,  0x99,  0x7E,  0x0C,  0x0, 0x0,  0x9F},                      //T_TECHM
      {0x28,  0x4D,  0xA3,  0x7E,  0x0C,  0x0, 0x0,  0x5A},                      //T_IOBOARD_D
      {0x28,  0x56,  0x99,  0x7E,  0x0C,  0x0, 0x0,  0xC0},                      //T_ELECON
      {0x28,  0x52,  0x99,  0x7E,  0x0C,  0x0, 0x0,  0x1C},                      //T_ELECON_D

};


/* Public variables */

/* Private methods */

/* The readout of sensor idx has finished: remember whether it was the 85.0 degC power-on value, log it (rate-limited).
 * Only looks at the received bytes, the value itself is taken over in TEMP_Update100ms() as before. */
static void CheckPowerOnValue(uint8_t idx)
{
	const uint8_t* sp = OW_GetScratchpad();
	sTemp* s = &mSensors[idx];
	uint8_t payload[6];
	uint32_t now;
	uint16_t minutes;

	s->por85 = (s->status == ets_Valid && sp[0] == 0x50 && sp[1] == 0x05) ? 1u : 0u;
	if (!s->por85)
	{
		return;
	}
	now = HAL_GetTick();
	if (mLogged85Once && (now - mLast85LogTick) < TEMP_85_LOG_MIN_MS)
	{
		return;
	}
	mLogged85Once = 1;
	mLast85LogTick = now;
	minutes = (uint16_t)(now / 60000u);
	payload[0] = TEMP_LOG_85_A;
	payload[1] = s->sensorId;
	payload[2] = (uint8_t)(mIgnored85 & 0xFF);
	payload[3] = (uint8_t)(mIgnored85 >> 8);
	payload[4] = (uint8_t)(minutes & 0xFF);
	payload[5] = (uint8_t)(minutes >> 8);
	LOG_SendEvent(eleLog, payload);
	payload[0] = TEMP_LOG_85_B;
	payload[1] = s->sensorId;
	payload[2] = sp[2];
	payload[3] = sp[4];
	payload[4] = sp[6];
	payload[5] = sp[7];
	LOG_SendEvent(eleLog, payload);
}

/* 85.0 degC power-on value: take it only if it is plausible (see TEMP.h) */
static uint8_t Accept85(sTemp* s)
{
	if (s->cnt85 < 255u)
	{
		s->cnt85++;
	}
	if (s->cnt85 >= TEMP_85_ACCEPT_REPEAT)
	{
		return 1;
	}
	return (s->tempC_10ths != (int16_t)0x8000) && (abs(s->tempC_10ths - TEMP_85_RAW_10THS) <= TEMP_85_ACCEPT_BAND);
}

/* Public methods */

void TEMP_Init(void)
{
	mTimer = 0;
	mReadId = 0;
	mConvertId = 0;
	mNumOfAssignedSensors = 0;
	mNumOfBuses = 0;
}

void TEMP_Update100ms(void)
{
	// cyclically read all assigned sensors and trig the conversion

	// the readout started in the last call has finished (it takes ~10 ms): check its scratchpad before the next
	// transfer overwrites it
	if (mPendingIdx >= 0 && !OW_IsBusy())
	{
		CheckPowerOnValue((uint8_t)mPendingIdx);
		mPendingIdx = -1;
	}

	if (mTimer == 0)
	{
		// copy results to Vars2 (by id, temperature in degC)
		int i;
		for(i = 0; i < mNumOfAssignedSensors; i++)
		{
			mSensors[i].errorCnt++;  // just assume error to simplify the code :-)
			// check the validity of last readout
			uint8_t ok = (mSensors[i].rawTempC_10ths >= -300) && (mSensors[i].rawTempC_10ths <= 1250) && mSensors[i].status == ets_Valid;  // valid range from -30 to 125 deg C
			if (ok && mSensors[i].por85)    // 85.0 degC power-on value: only if plausible, else keep the last value
			{
				if (!Accept85(&mSensors[i]))
				{
					ok = 0;
					if (mIgnored85 < 0xFFFFu)
					{
						mIgnored85++;
					}
				}
			}
			else if (ok)
			{
				mSensors[i].cnt85 = 0;
			}
			if (ok)
			{
				mSensors[i].errorCnt = 0;
				mSensors[i].tempC_10ths = mSensors[i].rawTempC_10ths;  // copy the raw value to valid value
				(void)VARS2_SetById(mSensors[i].VarId, (float)mSensors[i].tempC_10ths / 10.0f, 1);
			}

			if (mSensors[i].errorCnt > MAX_ERR_TO_INVALIDATE)
			{
				mSensors[i].errorCnt = 0;
				(void)VARS2_SetById(mSensors[i].VarId, (float)mSensors[i].tempC_10ths / 10.0f, 0);  // last valid value, invalid
				// TBD LOG ERR
			}

		}
		// trig conversion

	}

	if (mTimer < mNumOfBuses)
	{
		OW_ConvertAll(mTimer);  //  convert one Bus
	}


	if (mTimer > 20)  // 2 sec delay
	{

		if (OW_ReadSensor(mSensors[mReadId].owBusId,
									&(mSensorsAddress[mSensors[mReadId].sensorId]),
									&(mSensors[mReadId].rawTempC_10ths),
									&(mSensors[mReadId].status)) == etr_OK)
		{
			mPendingIdx = (int8_t)mReadId;
		}
		mReadId++;

	}
	mTimer++;

	if (mReadId >= mNumOfAssignedSensors)
	{
		mReadId = 0;
		mTimer = 0;
	}

}

uint8_t TEMP_AddHwBus(uint8_t busId, GPIO_TypeDef* port, uint32_t pin)
{
	if (mNumOfBuses < MAX_NUM_OF_BUSES)
	{
		OW_AddBus(busId,port, pin);
		mNumOfBuses ++;
		return 0;
	}
	else
	{
		return 1;
	}

}



uint8_t TEMP_AssignSensor(uint8_t sensorId, eVarId varId, uint8_t busId)
{
	if (mNumOfAssignedSensors < NUM_OF_ALL_SENSORS)
	{
		mSensors[mNumOfAssignedSensors].sensorId = sensorId;
		mSensors[mNumOfAssignedSensors].tempC_10ths = 0x8000;
		mSensors[mNumOfAssignedSensors].status = ets_NotValid;
		mSensors[mNumOfAssignedSensors].VarId = varId;
		mSensors[mNumOfAssignedSensors].owBusId = busId;
		mSensors[mNumOfAssignedSensors].errorCnt = 0;
		mSensors[mNumOfAssignedSensors].por85 = 0;
		mSensors[mNumOfAssignedSensors].cnt85 = 0;
		mNumOfAssignedSensors++;
		return 0;
	}
	else
	{
		return 1;
	}
}

uint16_t TEMP_GetIgnored85Count(void)
{
	return mIgnored85;
}
