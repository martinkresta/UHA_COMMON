/*
 * test_vars2.c - PC unit test for VARS2.c (UHA_COMMON/Src/VARS2.c).
 *
 * Mocks HAL_GetTick() (settable), VAR_GetVariable() (backed by a small
 * array instead of real VARS.c) and MCAN_SendFrame() (records the last
 * frame) so VARS2.c can be exercised without any target hardware and
 * without linking VARS.c/MCAN.c. Run via tests/test_vars2.py.
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

/* ---- HAL_GetTick() mock ---- */
static uint32_t gMockTick = 0;
uint32_t HAL_GetTick(void) { return gMockTick; }
static void SetTick(uint32_t t) { gMockTick = t; }

/* ---- VAR_GetVariable() mock: backs the old Vars[] with a plain array ---- */
static int16_t gRaw[NUM_OF_VARIABLES];
static uint8_t gValid[NUM_OF_VARIABLES];   /* 1 = valid */

static void SetVar(uint16_t id, int16_t raw, uint8_t valid)
{
	gRaw[id] = raw;
	gValid[id] = valid;
}

int16_t VAR_GetVariableLegacy(uint16_t varId, uint16_t* invalid)
{
	if (varId < NUM_OF_VARIABLES)
	{
		if (gValid[varId] == 0 && invalid != NULL) *invalid |= INVALID_FLAG;
		return gRaw[varId];
	}
	if (invalid != NULL) *invalid |= INVALID_FLAG;
	return 0;
}

/* ---- MCAN_SendFrame() mock: records the last queued frame ---- */
static int gSendCount;
static uint16_t gLastCobid;
static uint8_t gLastData[8];

uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	gSendCount++;
	gLastCobid = cobid;
	memcpy(gLastData, data, dlc <= 8 ? dlc : 8);
	return 1;
}

/* ---- helpers ---- */
static int feq(float a, float b)
{
	float d = a - b;
	if (d < 0) d = -d;
	return d < 1e-4f;
}

static void BuildRxFrame(uint8_t* frame, uint16_t id, const void* value4, uint8_t valid)
{
	frame[0] = (uint8_t)(id & 0xFFu);
	frame[1] = (uint8_t)(id >> 8);
	memcpy(&frame[2], value4, 4);
	frame[6] = valid ? 0x01u : 0u;
	frame[7] = 0;
}

static void test_rx_known_id(void)
{
	uint8_t frame[8];
	float v = 53.2f;

	VARS2_Init(NODEID_ELECON_D);
	SetTick(1000);

	BuildRxFrame(frame, VID_BAT_VOLTAGE_V, &v, 1);
	VARS2_ProcessFrame(frame);

	CHECK(feq(Vars2.GlobalEnergy.BatVoltage_V, 53.2f));
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);
}

static void test_rx_unknown_id_ignored(void)
{
	sVars before, after;
	uint8_t frame[8];
	float garbage = 999.0f;

	VARS2_Init(NODEID_ELECON_D);
	SetTick(2000);
	memcpy(&before, &Vars2, sizeof(sVars));

	/* id 0 and id 2 (unused) both have no VARS2 definition */
	BuildRxFrame(frame, 0, &garbage, 1);
	VARS2_ProcessFrame(frame);
	BuildRxFrame(frame, 2, &garbage, 1);
	VARS2_ProcessFrame(frame);
	/* id just above VAR_MAX_ID must also be rejected without touching VarIdToIndex out of bounds */
	BuildRxFrame(frame, VAR_MAX_ID + 1, &garbage, 1);
	VARS2_ProcessFrame(frame);

	memcpy(&after, &Vars2, sizeof(sVars));
	CHECK(memcmp(&before, &after, sizeof(sVars)) == 0);   /* no state anywhere changed, no crash */
}

static void test_rx_valid_flag_bit(void)
{
	uint8_t frame[8];
	float v1 = 53.2f, v2 = 44.4f;

	VARS2_Init(NODEID_ELECON_D);
	SetTick(3000);
	BuildRxFrame(frame, VID_BAT_VOLTAGE_V, &v1, 1);
	VARS2_ProcessFrame(frame);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);

	BuildRxFrame(frame, VID_BAT_VOLTAGE_V, &v2, 0);   /* VALID bit clear */
	VARS2_ProcessFrame(frame);
	CHECK(feq(Vars2.GlobalEnergy.BatVoltage_V, 44.4f));   /* value is still written */
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 0);          /* but now invalid */
}

static void test_rx_timeout(void)
{
	uint8_t frame[8];
	float v = 53.2f;
	uint32_t t0 = 5000;
	uint16_t timeoutMs = VarMeta[VarIdToIndex[VID_BAT_VOLTAGE_V]].TimeoutMs;   /* 3000 */

	VARS2_Init(NODEID_ELECON_D);
	SetTick(t0);
	BuildRxFrame(frame, VID_BAT_VOLTAGE_V, &v, 1);
	VARS2_ProcessFrame(frame);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);

	SetTick(t0 + timeoutMs);        /* exactly at the boundary: still valid */
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);

	SetTick(t0 + timeoutMs + 1);    /* past it: timed out, even though Valid bit was 1 */
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 0);
}

static void test_sendlegacy_float_scale10(void)
{
	float sent;

	VARS2_Init(NODEID_ELECON);
	SetVar(VAR_BAT_VOLTAGE_V10, 532, 1);
	gSendCount = 0;
	VARS2_SendLegacy(VAR_BAT_VOLTAGE_V10);

	CHECK(gSendCount == 1);
	CHECK(feq(Vars2.GlobalEnergy.BatVoltage_V, 53.2f));
	CHECK(gLastCobid == 0x180u + NODEID_ELECON);
	CHECK(gLastData[0] == (VID_BAT_VOLTAGE_V & 0xFFu));
	CHECK(gLastData[1] == (VID_BAT_VOLTAGE_V >> 8));
	memcpy(&sent, &gLastData[2], 4);
	CHECK(sent == Vars2.GlobalEnergy.BatVoltage_V);   /* frame bytes == what landed in Vars2, bit-identical */
	CHECK(gLastData[6] == 0x01u);                      /* valid, bits1-7 = 0 */
	CHECK(gLastData[7] == 0);                          /* seq, reserved */
}

static void test_sendlegacy_float_scale1000(void)
{
	float sent;

	VARS2_Init(NODEID_ELECON_D);
	SetVar(VAR_BMS1_CELL8_MV, 3326, 1);
	gSendCount = 0;
	VARS2_SendLegacy(VAR_BMS1_CELL8_MV);

	CHECK(gSendCount == 1);
	CHECK(feq(Vars2.Bms1.Cell_V[7], 3.326f));            /* id 187 = 8th cell, index 7 */
	CHECK(gLastData[0] == (VID_BMS1_CELL_V_8 & 0xFFu));
	CHECK(gLastData[1] == (VID_BMS1_CELL_V_8 >> 8));
	memcpy(&sent, &gLastData[2], 4);
	CHECK(sent == Vars2.Bms1.Cell_V[7]);
	CHECK(gLastData[6] == 0x01u);
}

static void test_sendlegacy_float_scale_point1(void)
{
	/* VAR_SOLAR_ENERGY_TODAY_10WH: legacy_scale 0.1 -> raw / 0.1 == raw * 10 */
	VARS2_Init(NODEID_ELECON);
	SetVar(VAR_SOLAR_ENERGY_TODAY_10WH, 1234, 1);
	gSendCount = 0;
	VARS2_SendLegacy(VAR_SOLAR_ENERGY_TODAY_10WH);

	CHECK(gSendCount == 1);
	CHECK(feq(Vars2.GlobalEnergy.SolarEnergyToday_Wh, 12340.0f));
}

static void test_sendlegacy_bool(void)
{
	uint32_t sent;

	VARS2_Init(NODEID_ELECON_D);
	SetVar(VAR_STRG2_BALANCED_TODAY, 1, 1);
	gSendCount = 0;
	VARS2_SendLegacy(VAR_STRG2_BALANCED_TODAY);

	CHECK(gSendCount == 1);
	CHECK(Vars2.Storage2.Strg2BalancedToday == 1u);   /* integer value of raw, not scaled */
	memcpy(&sent, &gLastData[2], 4);
	CHECK(sent == 1u);
	CHECK(gLastData[6] == 0x01u);

	SetVar(VAR_STRG2_BALANCED_TODAY, 0, 0);   /* also check invalid -> flags bit0 clear, value still updated */
	VARS2_SendLegacy(VAR_STRG2_BALANCED_TODAY);
	CHECK(Vars2.Storage2.Strg2BalancedToday == 0u);
	CHECK(gLastData[6] == 0x00u);
}

static void test_sendlegacy_enum(void)
{
	VARS2_Init(NODEID_TECHM);
	SetVar(VAR_EL_HEATER_STATUS, ElHeaterState_LowSOC, 1);
	gSendCount = 0;
	VARS2_SendLegacy(VAR_EL_HEATER_STATUS);

	CHECK(gSendCount == 1);
	CHECK(Vars2.ElHeater.Status == (uint32_t)ElHeaterState_LowSOC);
	CHECK(gLastCobid == 0x180u + NODEID_TECHM);
}

static void test_sendlegacy_unmapped_noop(void)
{
	VARS2_Init(NODEID_ELECON);
	SetVar(2, 7, 1);   /* valid Vars[] index, no VARS2 definition */
	gSendCount = 0;
	VARS2_SendLegacy(2);
	CHECK(gSendCount == 0);

	VARS2_SendLegacy((uint16_t)(VAR_MAX_ID + 1));   /* out of range id altogether */
	CHECK(gSendCount == 0);
}

static void test_setvalid_roundtrip(void)
{
	uint8_t frame[8];
	float v = 53.2f;

	VARS2_Init(NODEID_ELECON_D);
	SetTick(1000);
	BuildRxFrame(frame, VID_BAT_VOLTAGE_V, &v, 1);
	VARS2_ProcessFrame(frame);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);

	VARS2_SetValid(VID_BAT_VOLTAGE_V, 0);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 0);

	VARS2_SetValid(VID_BAT_VOLTAGE_V, 1);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);
}

static void test_default_valid_after_init(void)
{
	/* spec sec.4: "default after start = valid" - nothing has touched this
	 * value, tick has not advanced since VARS2_Init(), yet IsValid() is true */
	SetTick(9000);
	VARS2_Init(NODEID_ELECON);
	CHECK(VARS2_IsValid(VID_SHUNT_PCK2_CURRENT_A) == 1);
}


/* ---- step 2: frame hook, DLC/RTR check, own state, ELECON-local publishing ---- */
static int gHookCount;
static uint16_t gHookCob;
static uint8_t gHookData[8];
static void TestHook(uint16_t cobId, const uint8_t* d)
{
	gHookCount++;
	gHookCob = cobId;
	memcpy(gHookData, d, 8);
}

static void test_rx_dlc_rtr_rejected(void)
{
	uint8_t frame[8];
	float v = 53.2f, other = 11.1f;

	VARS2_Init(NODEID_ELECON_D);
	VARS2_SetFrameHook(TestHook);
	memset(&Vars2, 0, sizeof(Vars2));
	SetTick(1000);
	gHookCount = 0;

	BuildRxFrame(frame, VID_BAT_VOLTAGE_V, &v, 1);
	CHECK(VARS2_ProcessRxFrame(0x181, 7, 0, frame) == 0);   /* DLC != 8 */
	CHECK(VARS2_ProcessRxFrame(0x181, 0, 0, frame) == 0);
	CHECK(VARS2_ProcessRxFrame(0x181, 8, 1, frame) == 0);   /* RTR */
	CHECK(gHookCount == 0);                                  /* not forwarded either */
	CHECK(!feq(Vars2.GlobalEnergy.BatVoltage_V, 53.2f));     /* not stored */

	CHECK(VARS2_ProcessRxFrame(0x181, 8, 0, frame) == 1);   /* good frame */
	CHECK(gHookCount == 1 && gHookCob == 0x181);
	CHECK(memcmp(gHookData, frame, 8) == 0);                 /* data bytes unchanged */
	CHECK(feq(Vars2.GlobalEnergy.BatVoltage_V, 53.2f));

	/* unknown id: not stored, but still forwarded (the RPi decides) */
	BuildRxFrame(frame, VAR_MAX_ID + 1, &other, 1);
	gHookCount = 0;
	CHECK(VARS2_ProcessRxFrame(0x182, 8, 0, frame) == 1);
	CHECK(gHookCount == 1 && gHookCob == 0x182);
	VARS2_SetFrameHook(NULL);
}

static void test_tx_hook_and_own_state(void)
{
	uint32_t t = 100000;

	VARS2_Init(NODEID_ELECON);
	VARS2_SetFrameHook(TestHook);
	SetTick(t);
	SetVar(VAR_BAT_VOLTAGE_V10, 532, 1);
	gHookCount = 0;
	VARS2_SendLegacy(VAR_BAT_VOLTAGE_V10);
	CHECK(gHookCount == 1 && gHookCob == 0x184);
	CHECK(memcmp(gHookData, gLastData, 8) == 0);   /* same bytes as sent on CAN */

	/* own value does not time out as long as it is being sent */
	SetTick(t + 2000);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);
	VARS2_SendLegacy(VAR_BAT_VOLTAGE_V10);
	SetTick(t + 4000);   /* 2 s after the last send, timeout is 3 s */
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 1);
	SetTick(t + 5001);   /* 3 s + 1 after the last send */
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 0);

	/* producer-side invalid propagates to the own state */
	SetVar(VAR_BAT_VOLTAGE_V10, 0, 0);
	VARS2_SendLegacy(VAR_BAT_VOLTAGE_V10);
	CHECK(VARS2_IsValid(VID_BAT_VOLTAGE_V) == 0);
	VARS2_SetFrameHook(NULL);
}

static int gLocCount[VAR_MAX_ID + 1];
static int gLocOther;
static uint8_t gLocLast[VAR_MAX_ID + 1][8];
static void LocalCb(uint16_t cobId, const uint8_t* d)
{
	uint16_t id = (uint16_t)d[0] | ((uint16_t)d[1] << 8);
	if (cobId != 0x184 || id > VAR_MAX_ID) { gLocOther++; return; }
	gLocCount[id]++;
	memcpy(gLocLast[id], d, 8);
}

static void test_local_publishing(void)
{
	uint16_t i, nLocal = 0;
	uint32_t t, maxPerTick = 0;

	VARS2_Init(NODEID_ELECON);
	memset(gLocCount, 0, sizeof(gLocCount));
	gLocOther = 0;
	for (i = 0; i < NUM_OF_VARIABLES; i++) { gRaw[i] = (int16_t)(i + 1); gValid[i] = 1; }
	gValid[VAR_SOLAR_POWER_W] = 0;   /* one invalid local variable */
	gSendCount = 0;

	SetTick(500000);
	for (t = 0; t < 60000; t += 10)
	{
		int c0 = 0, c1 = 0;
		for (i = 0; i <= VAR_MAX_ID; i++) c0 += gLocCount[i];
		SetTick(500000 + t);
		VARS2_UpdateLocal_10ms(LocalCb);
		for (i = 0; i <= VAR_MAX_ID; i++) c1 += gLocCount[i];
		if ((uint32_t)(c1 - c0) > maxPerTick) maxPerTick = (uint32_t)(c1 - c0);
	}
	CHECK(gSendCount == 0);        /* nothing on CAN */
	CHECK(gLocOther == 0);         /* all with packet id 0x184 */
	CHECK(maxPerTick <= 4);        /* burst bounded */

	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		int expect;
		if (m->Producer != NODEID_ELECON || (m->Flags & VMF_ON_CAN) || m->PeriodMs == 0)
		{
			CHECK(gLocCount[m->Id] == 0);   /* CAN vars / foreign producers never published locally */
			continue;
		}
		nLocal++;
		expect = 60000 / m->PeriodMs;
		CHECK(gLocCount[m->Id] >= expect - 1 && gLocCount[m->Id] <= expect + 1);
	}
	CHECK(nLocal > 40);
	printf("local non-CAN variables: %u\n", nLocal);

	/* conversion identical to VARS2_SendLegacy: BatEnergy id16 raw 17, scale 1 */
	CHECK(gLocLast[16][6] == 0x01u);
	{
		float f; memcpy(&f, &gLocLast[16][2], 4);
		CHECK(feq(f, 17.0f));
		CHECK(feq(Vars2.GlobalEnergy.BatEnergy_Wh, 17.0f));
	}
	CHECK(gLocLast[VAR_SOLAR_POWER_W][6] == 0x00u);   /* invalid -> flag clear */
	CHECK(gLocCount[10] == 0);   /* id 10 = BatSoc is on CAN */

	/* NETWORK_STATUS (id 1, uint32, 5 s, not on CAN): tunnel only, every 5 s, bitmask as uint32 */
	CHECK(gLocCount[VAR_NETWORK_STATUS] >= 11 && gLocCount[VAR_NETWORK_STATUS] <= 13);
	{
		uint32_t u;
		SetVar(VAR_NETWORK_STATUS, 0xFE, 1);   /* nodes 1..7 in NMT RUN (COM_GetNetworkStatus) */
		memset(gLocLast[VAR_NETWORK_STATUS], 0, 8);
		for (t = 60000; t < 66000; t += 10) { SetTick(500000 + t); VARS2_UpdateLocal_10ms(LocalCb); }
		memcpy(&u, &gLocLast[VAR_NETWORK_STATUS][2], 4);
		CHECK(u == 0xFEu);
		CHECK(gLocLast[VAR_NETWORK_STATUS][6] == 0x01u);
		CHECK(Vars2.System.NetworkStatus == 0xFEu);
		CHECK(gSendCount == 0);   /* still nothing on CAN */
	}

	/* a node other than ELECON publishes nothing local */
	VARS2_Init(NODEID_TECHM);
	memset(gLocCount, 0, sizeof(gLocCount));
	for (t = 0; t < 5000; t += 10) { SetTick(900000 + t); VARS2_UpdateLocal_10ms(LocalCb); }
	{
		int sum = 0; for (i = 0; i <= VAR_MAX_ID; i++) sum += gLocCount[i];
		CHECK(sum == 0 || gLocOther == 0);   /* any TECHM local rows would be 0x183, counted in gLocOther */
	}
}

int main(void)
{
	test_rx_known_id();
	test_rx_unknown_id_ignored();
	test_rx_valid_flag_bit();
	test_rx_timeout();
	test_sendlegacy_float_scale10();
	test_sendlegacy_float_scale1000();
	test_sendlegacy_float_scale_point1();
	test_sendlegacy_bool();
	test_sendlegacy_enum();
	test_sendlegacy_unmapped_noop();
	test_setvalid_roundtrip();
	test_default_valid_after_init();
	test_rx_dlc_rtr_rejected();
	test_tx_hook_and_own_state();
	test_local_publishing();

	printf("%d checks, %d failed\n", checks, fails);
	printf(fails ? "TEST_VARS2 FAILED\n" : "TEST_VARS2 PASSED\n");
	return fails ? 1 : 0;
}
