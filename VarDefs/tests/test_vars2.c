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

int16_t VAR_GetVariable(uint16_t varId, uint16_t* invalid)
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

	/* id 0 and id 1 (VAR_NETWORK_STATUS) both have no VARS2 definition */
	BuildRxFrame(frame, 0, &garbage, 1);
	VARS2_ProcessFrame(frame);
	BuildRxFrame(frame, VAR_NETWORK_STATUS, &garbage, 1);
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
	SetVar(VAR_NETWORK_STATUS, 7, 1);   /* valid old VARS.h id, no VARS2 definition */
	gSendCount = 0;
	VARS2_SendLegacy(VAR_NETWORK_STATUS);
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

	printf("%d checks, %d failed\n", checks, fails);
	printf(fails ? "TEST_VARS2 FAILED\n" : "TEST_VARS2 PASSED\n");
	return fails ? 1 : 0;
}
