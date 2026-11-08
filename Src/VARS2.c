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
	uint8_t  Received;   /* 1 after the first frame received by VARS2_ProcessFrame() */
	uint32_t RxTick;
} sVars2State;

static sVars2State mState[VARS_COUNT];  /* indexed like VarMeta[]/VarIdToIndex[] */
static uint8_t mThisNodeId;
static VARS2_FrameHook mFrameHook;   /* NULL = no hook (all nodes except ELECON) */

/* ELECON-local (not on CAN) publishing: low 16 bits of last send tick per row */
#define VARS2_LOCAL_MAX_PER_CALL  4u   /* bounds the burst per 10 ms tick */
static uint16_t mLocalTick[VARS_COUNT];
static uint8_t  mLocalInit;


void VARS2_Init(uint8_t thisNodeId)
{
	uint16_t i;
	uint32_t now = HAL_GetTick();

	mThisNodeId = thisNodeId;
	for (i = 0; i < VARS_COUNT; i++)
	{
		mState[i].Valid = 1;   /* default after start = valid, spec sec.4 */
		mState[i].Received = 0;
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
	mState[idx].Received = 1;
	mState[idx].RxTick = HAL_GetTick();
}


/* Converts the old Vars[] value (+ validity) of row idx into a VARS2 frame,
 * stores the converted value into Vars2 and refreshes the producer's own
 * state (so VARS2_IsValid() of an own value does not time out). */
static void BuildLegacyFrame(uint16_t idx, uint16_t legacyVarId, uint8_t* data)
{
	uint16_t invalid = 0;
	int16_t raw;
	uint8_t valid;
	float f32;
	int32_t i32;
	const sVarMeta* meta = &VarMeta[idx];

	raw = VAR_GetVariableLegacy(legacyVarId, &invalid);   /* transmission always publishes Vars[] */
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

	mState[idx].Valid = valid;
	mState[idx].RxTick = HAL_GetTick();

	data[0] = (uint8_t)(meta->Id & 0xFFu);
	data[1] = (uint8_t)(meta->Id >> 8);
	memcpy(&data[2], meta->pValue, 4);
	data[6] = valid ? VARS2_FLAG_VALID : 0;
	data[7] = 0;   /* seq, reserved */
}


void VARS2_SendLegacy(uint16_t legacyVarId)
{
	uint8_t data[8];
	uint16_t idx;

	if (legacyVarId > VAR_MAX_ID)
	{
		return;   /* no VARS2 definition yet */
	}
	idx = VarIdToIndex[legacyVarId];   /* legacy id == VARS2 id in this migration step */
	if (idx == VAR_IDX_UNKNOWN)
	{
		return;
	}

	BuildLegacyFrame(idx, legacyVarId, data);

	MCAN_SendFrame(VARS2_COBID_BASE + mThisNodeId, data, 8);
	if (mFrameHook != NULL)
	{
		mFrameHook(VARS2_COBID_BASE + mThisNodeId, data);
	}
}


void VARS2_SetFrameHook(VARS2_FrameHook hook)
{
	mFrameHook = hook;
}


uint8_t VARS2_ProcessRxFrame(uint16_t cobId, uint8_t dlc, uint8_t rtr, const uint8_t* data)
{
	if (dlc != 8 || rtr)
	{
		return 0;   /* malformed: dropped, neither stored nor forwarded */
	}
	VARS2_ProcessFrame(data);
	if (mFrameHook != NULL)
	{
		mFrameHook(cobId, data);
	}
	return 1;
}


void VARS2_UpdateLocal_10ms(VARS2_FrameHook cb)
{
	uint16_t idx;
	uint16_t k = 0;
	uint8_t sent = 0;
	uint32_t now = HAL_GetTick();
	uint8_t data[8];

	if (cb == NULL)
	{
		return;
	}

	for (idx = 0; idx < VARS_COUNT; idx++)
	{
		const sVarMeta* meta = &VarMeta[idx];

		if (meta->Producer != mThisNodeId || (meta->Flags & VMF_ON_CAN) ||
		    meta->PeriodMs == 0 || meta->LegacyId == VAR_LEGACY_NONE)
		{
			continue;
		}

		if (mLocalInit == 0)
		{
			/* first call: spread the first sends, k-th row due k*10 ms from now */
			mLocalTick[idx] = (uint16_t)(now - meta->PeriodMs + (uint16_t)((k * 10u) % meta->PeriodMs));
			k++;
			continue;
		}

		if ((uint16_t)((uint16_t)now - mLocalTick[idx]) >= meta->PeriodMs && sent < VARS2_LOCAL_MAX_PER_CALL)
		{
			BuildLegacyFrame(idx, meta->LegacyId, data);
			mLocalTick[idx] = (uint16_t)now;
			cb(VARS2_COBID_BASE + mThisNodeId, data);
			sent++;
		}
	}
	mLocalInit = 1;
}


uint8_t VARS2_GetLegacy(uint16_t legacyVarId, int16_t* value, uint16_t* invalid)
{
	uint16_t idx;
	const sVarMeta* meta;
	float f32;
	int32_t i32;

	if (legacyVarId > VAR_MAX_ID)
	{
		return 0;
	}
	idx = VarIdToIndex[legacyVarId];   /* legacy id == VARS2 id in this migration step */
	if (idx == VAR_IDX_UNKNOWN)
	{
		return 0;
	}
	meta = &VarMeta[idx];
	if (meta->Producer == mThisNodeId || mState[idx].Received == 0)
	{
		return 0;
	}

	if (meta->Type == VT_FLOAT)
	{
		memcpy(&f32, meta->pValue, 4);
		f32 *= meta->LegacyScale;
		i32 = (int32_t)(f32 >= 0 ? f32 + 0.5f : f32 - 0.5f);   /* round to nearest, no libm */
	}
	else   /* integer value, as BuildLegacyFrame() writes it */
	{
		memcpy(&i32, meta->pValue, 4);
	}
	if (i32 > 32767) i32 = 32767;
	if (i32 < -32768) i32 = -32768;
	*value = (int16_t)i32;

	if (!VARS2_IsValid((eVarId)meta->Id) && invalid != NULL)
	{
		*invalid |= INVALID_FLAG;
	}
	return 1;
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
