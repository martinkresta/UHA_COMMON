/*
 * test_readlayer.c - PC unit test of the VARS2 read layer (step 6a, part A):
 * VAR_GetVariable() / VAR_EnableVars2Read() (real VARS.c) on top of
 * VARS2_GetLegacy() (real VARS2.c). HAL_GetTick() and MCAN_SendFrame() are
 * mocked. Run via tests/test_readlayer.py.
 */

#include <stdio.h>
#include <string.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "VARS.h"
#include "MCAN.h"

static int fails = 0;
static int checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static uint32_t gTick;
uint32_t HAL_GetTick(void) { return gTick; }

static uint8_t gFrame[8];
static int gSent;
uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	(void)cobid; (void)dlc;
	memcpy(gFrame, data, 8);
	gSent++;
	return 1;
}

#define SENTINEL 777

/* a frame as produced by node `producer` for legacy variable `id` with old value raw */
static void MakeFrame(const sVarMeta* m, int16_t raw, uint8_t valid)
{
	VARS2_Init(m->Producer);
	VAR_SetVariable(m->Id, raw, valid);
	gSent = 0;
	VARS2_SendLegacy(m->Id);
}

static uint8_t OtherNode(uint8_t producer) { return producer == NODEID_TECHM ? NODEID_ELECON : NODEID_TECHM; }

static void test_disabled_layer(void)
{
	const sVarMeta* m = &VarMeta[VarIdToIndex[VID_BAT_VOLTAGE_V]];
	uint16_t inv = 0;

	MakeFrame(m, 532, 1);
	VARS2_Init(OtherNode(m->Producer));
	VAR_SetVariable(m->Id, SENTINEL, 1);
	VARS2_ProcessFrame(gFrame);                       /* received, but layer not enabled */
	CHECK(VAR_GetVariable(m->Id, &inv) == SENTINEL);  /* old behaviour */
	CHECK(inv == 0);
	CHECK(VAR_GetVariableLegacy(m->Id, &inv) == SENTINEL);
}

static void test_roundtrip_all_rows(void)
{
	static const int16_t raws[] = { 0, 1, -1, 5, -5, 9, 123, -123, 1234, -1234, 3326, 32767, -32768, 10000, -10000 };
	uint16_t i;
	unsigned k, n = 0;
	int bad = 0;

	VAR_EnableVars2Read();
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		for (k = 0; k < sizeof(raws) / sizeof(raws[0]); k++)
		{
			uint16_t inv = 0;
			int16_t got;
			MakeFrame(m, raws[k], 1);
			VARS2_Init(OtherNode(m->Producer));
			VAR_SetVariable(m->Id, SENTINEL, 1);
			VARS2_ProcessFrame(gFrame);
			got = VAR_GetVariable(m->Id, &inv);
			n++;
			if (got != raws[k] || inv != 0)
			{
				if (bad < 10) printf("  mismatch id %u scale %g raw %d -> %d inv %u\n", m->Id, (double)m->LegacyScale, raws[k], got, inv);
				bad++;
			}
		}
	}
	printf("round trip: %u values, %d mismatches\n", n, bad);
	CHECK(bad == 0);
}

static void test_fallbacks(void)
{
	const sVarMeta* m = &VarMeta[VarIdToIndex[VID_BAT_VOLTAGE_V]];
	uint16_t inv = 0;

	VAR_EnableVars2Read();

	/* own variable: Vars[] even if a frame with the same id arrives */
	MakeFrame(m, 532, 1);
	VARS2_Init(m->Producer);
	VAR_SetVariable(m->Id, SENTINEL, 1);
	VARS2_ProcessFrame(gFrame);
	CHECK(VAR_GetVariable(m->Id, &inv) == SENTINEL && inv == 0);

	/* unknown id (VAR_NETWORK_STATUS has no VARS2 row) and out-of-range id */
	VARS2_Init(NODEID_TECHM);
	VAR_SetVariable(VAR_NETWORK_STATUS, 31, 1);
	CHECK(VAR_GetVariable(VAR_NETWORK_STATUS, &inv) == 31 && inv == 0);
	CHECK(VAR_GetVariable((uint16_t)(VAR_MAX_ID + 1), &inv) == 0 && inv == INVALID_FLAG);

	/* nothing received yet: Vars[] incl. its own (invalid) flag, exactly as today */
	inv = 0;
	VARS2_Init(OtherNode(m->Producer));
	VAR_SetVariable(m->Id, 520, 0);
	CHECK(VAR_GetVariable(m->Id, &inv) == 520);
	CHECK(inv == INVALID_FLAG);
	VAR_SetVariable(m->Id, 520, 1);
	inv = 0;
	CHECK(VAR_GetVariable(m->Id, &inv) == 520 && inv == 0);
}

static void test_invalid_and_timeout(void)
{
	const sVarMeta* m = &VarMeta[VarIdToIndex[VID_BAT_VOLTAGE_V]];
	uint16_t inv;
	uint8_t frame[8];

	VAR_EnableVars2Read();
	gTick = 10000;

	/* valid frame */
	MakeFrame(m, 532, 1);
	memcpy(frame, gFrame, 8);
	VARS2_Init(OtherNode(m->Producer));
	VAR_SetVariable(m->Id, SENTINEL, 1);
	VARS2_ProcessFrame(frame);
	inv = 0x80;   /* other bits are kept */
	CHECK(VAR_GetVariable(m->Id, &inv) == 532 && inv == 0x80);

	/* invalid flag in the frame: value still returned, INVALID_FLAG set */
	MakeFrame(m, 540, 0);
	memcpy(frame, gFrame, 8);
	VARS2_Init(OtherNode(m->Producer));
	VARS2_ProcessFrame(frame);
	inv = 0;
	CHECK(VAR_GetVariable(m->Id, &inv) == 540);
	CHECK((inv & INVALID_FLAG) != 0);

	/* NULL invalid pointer is accepted, with a valid and with an invalid value */
	CHECK(VAR_GetVariable(m->Id, NULL) == 540);
	MakeFrame(m, 541, 1);
	memcpy(frame, gFrame, 8);
	VARS2_Init(OtherNode(m->Producer));
	VARS2_ProcessFrame(frame);
	CHECK(VAR_GetVariable(m->Id, NULL) == 541);

	/* timeout: last value kept, flagged invalid after TimeoutMs */
	gTick = 20000;
	VARS2_Init(OtherNode(m->Producer));
	VARS2_ProcessFrame(frame);
	gTick += m->TimeoutMs;
	inv = 0;
	CHECK(VAR_GetVariable(m->Id, &inv) == 541 && inv == 0);       /* boundary still valid */
	gTick += 1;
	CHECK(VAR_GetVariable(m->Id, &inv) == 541);
	CHECK((inv & INVALID_FLAG) != 0);
	CHECK(VAR_GetVariable(m->Id, NULL) == 541);

	/* a new frame revives it */
	VARS2_ProcessFrame(frame);
	inv = 0;
	CHECK(VAR_GetVariable(m->Id, &inv) == 541 && inv == 0);
}

static void test_transmission_uses_legacy(void)
{
	/* ELECON receives BMS1 SOC (producer ELECON_D) and sends its own: the sent frame
	 * must carry Vars[], not the value received into Vars2 */
	const sVarMeta* m = &VarMeta[VarIdToIndex[VID_BMS1_SOC]];
	uint8_t frame[8];
	float f;

	VAR_EnableVars2Read();
	MakeFrame(m, 80, 1);
	memcpy(frame, gFrame, 8);
	VARS2_Init(NODEID_ELECON);
	VARS2_ProcessFrame(frame);            /* Vars2 now holds 80.0 */
	VAR_SetVariable(m->Id, 55, 1);        /* old Vars[] differs */
	gSent = 0;
	VARS2_SendLegacy(m->Id);              /* wrong producer, but it must read Vars[] */
	CHECK(gSent == 1);
	memcpy(&f, &gFrame[2], 4);
	CHECK(f == 55.0f);
}

int main(void)
{
	test_disabled_layer();       /* must run before the layer is enabled */
	test_roundtrip_all_rows();
	test_fallbacks();
	test_invalid_and_timeout();
	test_transmission_uses_legacy();
	printf("%d checks, %d failed\n", checks, fails);
	printf(fails ? "TEST_READLAYER FAILED\n" : "TEST_READLAYER PASSED\n");
	return fails ? 1 : 0;
}
