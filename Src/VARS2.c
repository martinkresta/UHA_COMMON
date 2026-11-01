/*
 * VARS2.c
 *
 * VARS2 shared variable protocol - migration step 1 (dual old+new TX, RX into
 * Vars2). See UHA_COMMON/VarDefs/VARS2_spec.md.
 *
 * Hand-written, NOT generated. Uses the generated tables in uha_vars_gen.h/.c.
 */

#include "VARS2.h"
#include "VARS.h"   /* legacy VAR_GetVariable(), INVALID_FLAG */
#include "MCAN.h"   /* MCAN_SendFrame() */
#include <string.h>

#define VARS2_COBID_BASE  0x180u
#define VARS2_FLAG_VALID  0x01u  /* spec sec.2, byte 6 bit0 */

typedef struct
{
	uint8_t  Valid;
	uint32_t RxTick;
} sVars2State;

static sVars2State mState[VARS_COUNT];  /* indexed like VarMeta[]/VarIdToIndex[] */
static uint8_t mThisNodeId;


void VARS2_Init(uint8_t thisNodeId)
{
	uint16_t i;
	uint32_t now = HAL_GetTick();

	mThisNodeId = thisNodeId;
	for (i = 0; i < VARS_COUNT; i++)
	{
		mState[i].Valid = 1;   /* default after start = valid, spec sec.4 */
		mState[i].RxTick = now;
	}
}


void VARS2_ProcessFrame(const uint8_t* data)
{
	uint16_t id = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
	uint16_t idx;

	if (id > VAR_MAX_ID)
	{
		return;   /* unknown id */
	}
	idx = VarIdToIndex[id];
	if (idx == VAR_IDX_UNKNOWN)
	{
		return;
	}

	memcpy(VarMeta[idx].pValue, &data[2], 4);  /* raw 4B copy, Cortex-M is LE like the frame */
	mState[idx].Valid = (data[6] & VARS2_FLAG_VALID) ? 1 : 0;
	mState[idx].RxTick = HAL_GetTick();
}


void VARS2_SendLegacy(uint16_t legacyVarId)
{
	uint16_t invalid = 0;
	int16_t raw;
	uint8_t valid;
	uint8_t data[8];
	float f32;
	int32_t i32;
	uint16_t idx;
	const sVarMeta* meta;

	if (legacyVarId > VAR_MAX_ID)
	{
		return;   /* no VARS2 definition yet */
	}
	idx = VarIdToIndex[legacyVarId];   /* legacy id == VARS2 id in this migration step */
	if (idx == VAR_IDX_UNKNOWN)
	{
		return;
	}
	meta = &VarMeta[idx];

	raw = VAR_GetVariable(legacyVarId, &invalid);
	valid = (invalid == INVALID_FLAG) ? 0 : 1;

	if (meta->Type == VT_FLOAT)
	{
		f32 = (float)raw / meta->LegacyScale;
		memcpy(meta->pValue, &f32, 4);
	}
	else   /* VT_INT32 / VT_UINT32 / VT_BOOL / VT_ENUM: integer value of raw */
	{
		i32 = (int32_t)raw;
		memcpy(meta->pValue, &i32, 4);
	}

	data[0] = (uint8_t)(meta->Id & 0xFFu);
	data[1] = (uint8_t)(meta->Id >> 8);
	memcpy(&data[2], meta->pValue, 4);
	data[6] = valid ? VARS2_FLAG_VALID : 0;
	data[7] = 0;   /* seq, reserved */

	MCAN_SendFrame(VARS2_COBID_BASE + mThisNodeId, data, 8);
}


void VARS2_SetValid(eVarId id, uint8_t valid)
{
	uint16_t idx;

	if ((uint16_t)id > VAR_MAX_ID)
	{
		return;
	}
	idx = VarIdToIndex[(uint16_t)id];
	if (idx == VAR_IDX_UNKNOWN)
	{
		return;
	}
	mState[idx].Valid = valid;
	mState[idx].RxTick = HAL_GetTick();
}


uint8_t VARS2_IsValid(eVarId id)
{
	uint16_t idx;

	if ((uint16_t)id > VAR_MAX_ID)
	{
		return 0;
	}
	idx = VarIdToIndex[(uint16_t)id];
	if (idx == VAR_IDX_UNKNOWN)
	{
		return 0;
	}
	if (mState[idx].Valid == 0)
	{
		return 0;
	}
	if ((HAL_GetTick() - mState[idx].RxTick) > VarMeta[idx].TimeoutMs)
	{
		return 0;
	}
	return 1;
}
