/*
 * VARS2.c
 *
 * VARS2 shared variable protocol: TX of this node's variables from Vars2 by
 * their YAML period (CAN 0x180+node, tunnel callback for on_can: false),
 * RX of the other nodes' variables into Vars2, receive timeout.
 * See UHA_COMMON/VarDefs/VARS2_spec.md.
 *
 * Hand-written, NOT generated. Uses the generated tables in uha_vars_gen.h/.c.
 * Not interrupt safe: call everything from the main loop (no node writes or
 * reads Vars2 in an interrupt; checked per node, direct-access-notes.md).
 */

#include "VARS2.h"
#include "MCAN.h"   /* MCAN_SendFrame() */
#include <string.h>

#define VARS2_COBID_BASE  0x180u
#define VARS2_FLAG_VALID  0x01u  /* spec sec.2, byte 6 bit0 */

#define VARS2_CAN_MAX_PER_CALL    2u   /* max 2 frames per 10 ms (tests: VARS2_TEST_CAN_MAX in test_vars2.c, test_com.c) */
#define VARS2_LOCAL_MAX_PER_CALL  4u   /* bounds the tunnel burst per 10 ms tick */

/* protocol state only; value + valid live in Vars2 */
typedef struct
{
	uint32_t RxTick;   /* received variables: last frame */
	uint16_t TxTick;   /* own variables: low 16 bits of the last send tick */
} sVars2State;

static sVars2State mState[VARS_COUNT];  /* indexed like VarMeta[]/VarIdToIndex[] */
static uint8_t mThisNodeId;
static VARS2_FrameHook mFrameHook;   /* NULL = no hook (all nodes except ELECON) */
static uint8_t mTxInit;              /* first VARS2_Update_10ms() spreads the CAN sends */
static uint8_t mLocalInit;           /* same for VARS2_UpdateLocal_10ms() */
static uint16_t mTxNext;             /* round robin start for the CAN TX scan */


static uint8_t* ValidOf(const sVarMeta* meta)
{
	return (uint8_t*)meta->pVar + VAR_VALID_OFFSET;
}

static const sVarMeta* MetaOf(uint16_t id)
{
	uint16_t idx;

	if (id > VAR_MAX_ID)
	{
		return NULL;
	}
	idx = VarIdToIndex[id];
	return (idx == VAR_IDX_UNKNOWN) ? NULL : &VarMeta[idx];
}

static uint8_t IsOwn(const sVarMeta* meta)
{
	return meta->Producer == mThisNodeId;
}


void VARS2_Init(uint8_t thisNodeId)
{
	uint16_t i;
	uint32_t now = HAL_GetTick();

	mThisNodeId = thisNodeId;   /* the frame hook is kept: ELECON installs it in SCOM_Init() before COM_Init() */
	mTxInit = 0;
	mLocalInit = 0;
	mTxNext = 0;
	memset(&Vars2, 0, sizeof Vars2);   /* all values 0, all invalid: nothing produced / received yet */
	for (i = 0; i < VARS_COUNT; i++)
	{
		mState[i].RxTick = now;
		mState[i].TxTick = (uint16_t)now;
	}
}


/* frame of row idx from Vars2: id LE, value 4 B LE, flags, seq */
static void BuildFrame(uint16_t idx, uint8_t* data)
{
	const sVarMeta* meta = &VarMeta[idx];

	data[0] = (uint8_t)(meta->Id & 0xFFu);
	data[1] = (uint8_t)(meta->Id >> 8);
	memcpy(&data[2], meta->pVar, 4);   /* value at offset 0, Cortex-M is LE like the frame */
	data[6] = *ValidOf(meta) ? VARS2_FLAG_VALID : 0u;
	data[7] = 0;   /* seq, reserved */
}

static void SendOnCan(uint16_t idx)
{
	uint8_t data[8];

	BuildFrame(idx, data);
	MCAN_SendFrame(VARS2_COBID_BASE + mThisNodeId, data, 8);
	if (mFrameHook != NULL)
	{
		mFrameHook(VARS2_COBID_BASE + mThisNodeId, data);
	}
}

/* own variable of the given kind (onCan = 1: on CAN, 0: tunnel only) with a period */
static uint8_t IsOwnTx(const sVarMeta* meta, uint8_t onCan)
{
	return IsOwn(meta) && meta->PeriodMs != 0 && (((meta->Flags & VMF_ON_CAN) ? 1u : 0u) == onCan);
}

/* first call: k-th row of the kind is due k*10 ms from now (spreads the first sends) */
static void SpreadFirstSends(uint8_t onCan, uint32_t now)
{
	uint16_t idx;
	uint16_t k = 0;

	for (idx = 0; idx < VARS_COUNT; idx++)
	{
		const sVarMeta* meta = &VarMeta[idx];

		if (IsOwnTx(meta, onCan))
		{
			mState[idx].TxTick = (uint16_t)(now - meta->PeriodMs + (uint16_t)((k * 10u) % meta->PeriodMs));
			k++;
		}
	}
}

static uint8_t IsDue(uint16_t idx, uint32_t now)
{
	return (uint16_t)((uint16_t)now - mState[idx].TxTick) >= VarMeta[idx].PeriodMs;
}


void VARS2_Update_10ms(void)
{
	uint16_t n;
	uint16_t idx;
	uint8_t sent = 0;
	uint32_t now = HAL_GetTick();

	/* receive timeout */
	for (idx = 0; idx < VARS_COUNT; idx++)
	{
		const sVarMeta* meta = &VarMeta[idx];

		if (!IsOwn(meta) && *ValidOf(meta) && (now - mState[idx].RxTick) > meta->TimeoutMs)
		{
			*ValidOf(meta) = 0;
		}
	}

	/* TX of own on_can variables, round robin so that no row starves */
	if (mTxInit == 0)
	{
		SpreadFirstSends(1, now);
		mTxInit = 1;
		return;
	}
	for (n = 0; n < VARS_COUNT && sent < VARS2_CAN_MAX_PER_CALL; n++)
	{
		idx = (uint16_t)((mTxNext + n) % VARS_COUNT);
		if (IsOwnTx(&VarMeta[idx], 1) && IsDue(idx, now))
		{
			SendOnCan(idx);
			mState[idx].TxTick = (uint16_t)now;
			mTxNext = (uint16_t)((idx + 1u) % VARS_COUNT);
			sent++;
		}
	}
}


void VARS2_UpdateLocal_10ms(VARS2_FrameHook cb)
{
	uint16_t idx;
	uint8_t sent = 0;
	uint32_t now = HAL_GetTick();
	uint8_t data[8];

	if (cb == NULL)
	{
		return;
	}
	if (mLocalInit == 0)
	{
		SpreadFirstSends(0, now);
		mLocalInit = 1;
		return;
	}

	for (idx = 0; idx < VARS_COUNT && sent < VARS2_LOCAL_MAX_PER_CALL; idx++)
	{
		if (IsOwnTx(&VarMeta[idx], 0) && IsDue(idx, now))
		{
			BuildFrame(idx, data);
			mState[idx].TxTick = (uint16_t)now;
			cb(VARS2_COBID_BASE + mThisNodeId, data);
			sent++;
		}
	}
}


void VARS2_SetFrameHook(VARS2_FrameHook hook)
{
	mFrameHook = hook;
}


uint8_t VARS2_ProcessRxFrame(uint16_t cobId, uint8_t dlc, uint8_t rtr, const uint8_t* data)
{
	const sVarMeta* meta;

	if (dlc != 8 || rtr)
	{
		return 0;   /* malformed: dropped, neither stored nor forwarded */
	}

	meta = MetaOf((uint16_t)data[0] | ((uint16_t)data[1] << 8));
	if (meta != NULL && !IsOwn(meta))   /* unknown ids ignored; own values never overwritten */
	{
		memcpy(meta->pVar, &data[2], 4);
		*ValidOf(meta) = (data[6] & VARS2_FLAG_VALID) ? 1u : 0u;
		mState[meta - VarMeta].RxTick = HAL_GetTick();
	}

	if (mFrameHook != NULL)
	{
		mFrameHook(cobId, data);
	}
	return 1;
}


void VARS2_SetGroupValidRaw(void* pGroup, size_t size, uint8_t valid)
{
	uint8_t* p = (uint8_t*)pGroup;
	size_t i;

	for (i = 0; i + VAR_ITEM_SIZE <= size; i += VAR_ITEM_SIZE)
	{
		p[i + VAR_VALID_OFFSET] = valid ? 1u : 0u;
	}
}


/* value in physical units into the 4-byte value of meta's field */
static void StoreValue(const sVarMeta* meta, float value)
{
	int32_t i32;

	if (meta->Type == VT_FLOAT)
	{
		memcpy(meta->pVar, &value, 4);
	}
	else if (meta->Type == VT_UINT32)
	{
		uint32_t u32 = (value <= 0.0f) ? 0u : (uint32_t)(value + 0.5f);
		memcpy(meta->pVar, &u32, 4);
	}
	else   /* VT_INT32 / VT_BOOL / VT_ENUM */
	{
		i32 = (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);   /* round to nearest, no libm */
		memcpy(meta->pVar, &i32, 4);
	}
}

uint8_t VARS2_SetById(eVarId id, float value, uint8_t valid)
{
	const sVarMeta* meta = MetaOf((uint16_t)id);

	if (meta == NULL || !IsOwn(meta))
	{
		return 0;
	}
	StoreValue(meta, value);
	*ValidOf(meta) = valid ? 1u : 0u;
	return 1;
}
