/*
 * test_vars2.c - PC unit test for VARS2.c (direct access: Vars2 with {value, valid}).
 *
 * Real VARS2.c + uha_vars_gen.c; HAL_GetTick() and MCAN_SendFrame() are
 * mocked. Covers RX into Vars2, receive timeout, own values never overwritten,
 * VARS2_SetGroupValid() on groups and containers, VarSet(), VARS2_SetById(),
 * YAML-driven TX (periods, 1 CAN frame per 10 ms, frame hook, tunnel callback)
 * and the legacy wrapper round trip. Run via tests/test_vars2.py.
 */

#include <stdio.h>
#include <string.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "MCAN.h"

static int fails = 0;
static int checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* ---- mocks ---- */
static uint32_t gTick;
uint32_t HAL_GetTick(void) { return gTick; }

#define MAXTX 4000
typedef struct { uint32_t tick; uint16_t cobid; uint8_t data[8]; } sTx;
static sTx gTx[MAXTX];
static int gTxN;
uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	if (gTxN < MAXTX)
	{
		gTx[gTxN].tick = gTick;
		gTx[gTxN].cobid = cobid;
		memcpy(gTx[gTxN].data, data, dlc <= 8 ? dlc : 8);
		gTxN++;
	}
	return 1;
}

static int gHookN;
static uint16_t gHookCobid;
static uint8_t gHookData[8];
static void Hook(uint16_t cobid, const uint8_t* data)
{
	gHookN++;
	gHookCobid = cobid;
	memcpy(gHookData, data, 8);
}

static int gLocN;
static uint16_t gLocIds[MAXTX];
static uint32_t gLocTick[MAXTX];
static void LocalCb(uint16_t cobid, const uint8_t* data)
{
	(void)cobid;
	if (gLocN < MAXTX)
	{
		gLocIds[gLocN] = (uint16_t)(data[0] | (data[1] << 8));
		gLocTick[gLocN] = gTick;
		gLocN++;
	}
}

/* ---- helpers ---- */
static const sVarMeta* Meta(uint16_t id) { return &VarMeta[VarIdToIndex[id]]; }

static void Frame(uint8_t* f, uint16_t id, const void* value4, uint8_t valid)
{
	f[0] = (uint8_t)(id & 0xFFu);
	f[1] = (uint8_t)(id >> 8);
	memcpy(&f[2], value4, 4);
	f[6] = valid ? 1u : 0u;
	f[7] = 0;
}

static void Reset(uint8_t node)
{
	gTick = 5000;
	gTxN = 0;
	gHookN = 0;
	gLocN = 0;
	VARS2_Init(node);
}

static void Run(uint32_t ms)
{
	uint32_t t;
	for (t = 0; t < ms; t += 10)
	{
		gTick += 10;
		VARS2_Update_10ms();
	}
}

/* ---- tests ---- */
static void test_init(void)
{
	uint16_t i;
	int allInvalid = 1;
	Vars2.GlobalEnergy.Battery.Soc_pct.value = 12.0f;
	Vars2.GlobalEnergy.Battery.Soc_pct.valid = 1;
	Reset(NODEID_TECHM);
	for (i = 0; i < VARS_COUNT; i++)
	{
		if (((const uint8_t*)VarMeta[i].pVar)[VAR_VALID_OFFSET] != 0) allInvalid = 0;
	}
	CHECK(allInvalid);
	CHECK(Vars2.GlobalEnergy.Battery.Soc_pct.value == 0.0f);
}

static void test_rx(void)
{
	uint8_t f[8];
	float v = 53.2f;
	int32_t e = 3;
	const sVarsGlobalEnergy* pGe = &Vars2.GlobalEnergy;   /* how a reader sees it */

	VARS2_SetFrameHook(Hook);
	Reset(NODEID_TECHM);   /* the hook survives VARS2_Init() (ELECON: SCOM_Init() before COM_Init()) */

	Frame(f, VID_BAT_SOC, &v, 1);
	CHECK(VARS2_ProcessRxFrame(0x184, 8, 0, f) == 1);
	CHECK(pGe->Battery.Soc_pct.value == 53.2f);
	CHECK(pGe->Battery.Soc_pct.valid == 1);
	CHECK(gHookN == 1 && gHookCobid == 0x184 && memcmp(gHookData, f, 8) == 0);

	v = -12.5f;
	Frame(f, VID_BAT_CURRENT_A, &v, 0);   /* invalid in the frame */
	VARS2_ProcessRxFrame(0x184, 8, 0, f);
	CHECK(pGe->Battery.Current_A.value == -12.5f && pGe->Battery.Current_A.valid == 0);

	/* integer type, array element, nested container */
	Frame(f, VID_STRG2_BALANCED_TODAY, &e, 1);
	VARS2_ProcessRxFrame(0x187, 8, 0, f);
	CHECK(Vars2.WorkshopEnergy.Storage.BalancedToday.value == 3 && Vars2.WorkshopEnergy.Storage.BalancedToday.valid == 1);
	v = 3.301f;
	Frame(f, VID_BMS3_CELL_V_16, &v, 1);
	VARS2_ProcessRxFrame(0x184, 8, 0, f);
	CHECK(Vars2.PackHome80Ah.CellVoltage_V[15].value == 3.301f && Vars2.PackHome80Ah.CellVoltage_V[15].valid == 1);

	/* own value is never overwritten, unknown id ignored, both still go to the hook */
	Vars2.Temperatures.Techm.Board_C.value = 31.5f;
	Vars2.Temperatures.Techm.Board_C.valid = 1;
	v = -99.0f;
	Frame(f, VID_TEMP_TECHM_BOARD_C, &v, 0);
	gHookN = 0;
	CHECK(VARS2_ProcessRxFrame(0x183, 8, 0, f) == 1);
	CHECK(Vars2.Temperatures.Techm.Board_C.value == 31.5f && Vars2.Temperatures.Techm.Board_C.valid == 1);
	Frame(f, 2, &v, 1);   /* id 2: no variable */
	CHECK(VARS2_ProcessRxFrame(0x184, 8, 0, f) == 1);
	CHECK(gHookN == 2);

	/* malformed: dropped, no hook */
	gHookN = 0;
	Frame(f, VID_BAT_SOC, &v, 1);
	CHECK(VARS2_ProcessRxFrame(0x184, 7, 0, f) == 0);
	CHECK(VARS2_ProcessRxFrame(0x184, 8, 1, f) == 0);
	CHECK(gHookN == 0 && pGe->Battery.Soc_pct.value == 53.2f);
	VARS2_SetFrameHook(NULL);
}

static void test_timeout(void)
{
	uint8_t f[8];
	float v = 20.0f;
	uint32_t tout = Meta(VID_TEMP_OUTSIDE_C)->TimeoutMs;   /* 9000 */

	Reset(NODEID_RECU);
	Frame(f, VID_TEMP_OUTSIDE_C, &v, 1);
	VARS2_ProcessRxFrame(0x184, 8, 0, f);
	Run(tout - 10);
	CHECK(Vars2.Temperatures.Elecon.Outside_C.valid == 1);
	Run(20);
	CHECK(Vars2.Temperatures.Elecon.Outside_C.valid == 0);
	CHECK(Vars2.Temperatures.Elecon.Outside_C.value == 20.0f);   /* last value kept */
	VARS2_ProcessRxFrame(0x184, 8, 0, f);                        /* a new frame: valid again */
	CHECK(Vars2.Temperatures.Elecon.Outside_C.valid == 1);

	/* own values do not time out */
	Vars2.Recu.Fans.Fresh_pct.valid = 1;
	Run(60000);
	CHECK(Vars2.Recu.Fans.Fresh_pct.valid == 1);
}

static void test_group_valid(void)
{
	sVarsGlobalEnergy* pGe = &Vars2.GlobalEnergy;
	sVarsPack* pPack = &Vars2.PackHome80Ah;
	int i, all;

	Reset(NODEID_ELECON);
	pGe->Battery.Soc_pct.value = 50.0f;
	VARS2_SetGroupValid(&pGe->Battery, 1);
	CHECK(pGe->Battery.Soc_pct.valid && pGe->Battery.Voltage_V.valid && pGe->Battery.Current_A.valid &&
	      pGe->Battery.Energy_Wh.valid && pGe->Battery.Power_W.valid);
	CHECK(!pGe->Solar.Power_W.valid && !pGe->Load.Current_A.valid);   /* neighbours untouched */
	CHECK(!Vars2.System.NetworkStatus.valid && !Vars2.PackWorkshop251Ah.Soc_pct.valid);
	CHECK(pGe->Battery.Soc_pct.value == 50.0f);                        /* values untouched */

	VARS2_SetGroupValid(pGe, 1);                                       /* whole container */
	CHECK(pGe->Solar.EnergyToday_Wh.valid && pGe->Load.Power_W.valid);
	CHECK(!Vars2.PackWorkshop251Ah.Soc_pct.valid && !Vars2.Mppt.BatCurrent_A.valid);
	VARS2_SetGroupValid(&pGe->Solar, 0);
	CHECK(!pGe->Solar.Power_W.valid && !pGe->Solar.EnergyToday_Wh.valid && pGe->Load.Power_W.valid);

	VARS2_SetGroupValid(pPack, 1);                                     /* arrays */
	for (all = 1, i = 0; i < 16; i++) all = all && pPack->CellVoltage_V[i].valid && pPack->CellTemp_C[i].valid;
	CHECK(all && pPack->Soc_pct.valid);
	CHECK(!Vars2.PackHome220Ah.CellTemp_C[0].valid && !Vars2.Mppt.BatCurrent_A.valid);

	VARS2_SetGroupValid(&Vars2.HomeEnergy, 1);
	VARS2_SetGroupValid(&Vars2.HomeEnergy.Load, 0);
	CHECK(Vars2.HomeEnergy.Storage.BalancedToday.valid && !Vars2.HomeEnergy.Load.Cons_Wh.valid &&
	      Vars2.HomeEnergy.Pv.Prod_Wh.valid && !Vars2.WorkshopEnergy.Storage.Soc_pct.valid);

	VarSet(&Vars2.ShuntHome.Current_A, -3.25f, 1);
	CHECK(Vars2.ShuntHome.Current_A.value == -3.25f && Vars2.ShuntHome.Current_A.valid == 1);
	VarSet(&Vars2.ShuntHome.Current_A, 1.0f, 0);
	CHECK(Vars2.ShuntHome.Current_A.value == 1.0f && Vars2.ShuntHome.Current_A.valid == 0);
}

static void test_set_by_id(void)
{
	Reset(NODEID_TECHM);
	CHECK(VARS2_SetById(VID_TEMP_TANK_3_C, 54.3f, 1) == 1);
	CHECK(Vars2.Temperatures.Techm.Tank3_C.value == 54.3f && Vars2.Temperatures.Techm.Tank3_C.valid == 1);
	CHECK(VARS2_SetById(VID_TEMP_TANK_3_C, -3276.8f, 0) == 1);
	CHECK(Vars2.Temperatures.Techm.Tank3_C.value == -3276.8f && Vars2.Temperatures.Techm.Tank3_C.valid == 0);
	CHECK(VARS2_SetById(VID_EL_HEATER_STATUS, 7.0f, 1) == 1);          /* enum */
	CHECK(Vars2.ElHeater.State.value == ElHeaterState_InvalidInputs && Vars2.ElHeater.State.valid == 1);
	CHECK(VARS2_SetById(VID_EL_HEATER_STATUS, 2.6f, 1) == 1);          /* rounded */
	CHECK(Vars2.ElHeater.State.value == 3);
	CHECK(VARS2_SetById(VID_EL_HEATER_STATUS, -2.6f, 1) == 1);
	CHECK(Vars2.ElHeater.State.value == -3);

	/* other node's variable and unknown id: nothing written */
	CHECK(VARS2_SetById(VID_TEMP_OUTSIDE_C, 5.0f, 1) == 0);
	CHECK(Vars2.Temperatures.Elecon.Outside_C.valid == 0 && Vars2.Temperatures.Elecon.Outside_C.value == 0.0f);
	CHECK(VARS2_SetById((eVarId)2, 5.0f, 1) == 0);
	CHECK(VARS2_SetById((eVarId)(VAR_MAX_ID + 1), 5.0f, 1) == 0);

	Reset(NODEID_ELECON);
	CHECK(VARS2_SetById(VID_NETWORK_STATUS, 158.0f, 1) == 1);          /* uint32 */
	CHECK(Vars2.System.NetworkStatus.value == 158u);
	CHECK(VARS2_SetById(VID_NETWORK_STATUS, -1.0f, 1) == 1);
	CHECK(Vars2.System.NetworkStatus.value == 0u);
}

static void test_tx(void)
{
	int perId[VAR_MAX_ID + 1];
	uint32_t last[VAR_MAX_ID + 1];
	int i, bad = 0, maxPerTick = 0, inTick = 0, frames;
	uint32_t prevTick = 0;
	uint16_t k;
	const uint8_t node = NODEID_ELECON_D;
	float v;

	Reset(node);
	VARS2_SetFrameHook(Hook);
	VARS2_SetGroupValid(&Vars2.Axpert, 1);
	Vars2.Axpert.BatVoltage_V.value = 52.7f;
	Vars2.WorkshopEnergy.Storage.Soc_pct.value = 81.5f;   /* stays invalid */
	Run(60000);
	memset(perId, 0, sizeof perId);
	memset(last, 0, sizeof last);
	for (i = 0; i < gTxN; i++)
	{
		const sTx* f = &gTx[i];
		uint16_t id = (uint16_t)(f->data[0] | (f->data[1] << 8));
		const sVarMeta* m;
		CHECK(f->cobid == 0x180u + node);
		if (id > VAR_MAX_ID || VarIdToIndex[id] == VAR_IDX_UNKNOWN) { CHECK(0); continue; }
		m = Meta(id);
		CHECK(m->Producer == node && (m->Flags & VMF_ON_CAN));
		if (perId[id] > 0 && f->tick - last[id] < m->PeriodMs) bad++;
		perId[id]++;
		last[id] = f->tick;
		if (f->tick == prevTick) inTick++; else inTick = 1;
		if (inTick > maxPerTick) maxPerTick = inTick;
		prevTick = f->tick;
		if (id == VID_AXPERT_BAT_VOLTAGE_V)
		{
			memcpy(&v, &f->data[2], 4);
			CHECK(v == 52.7f && f->data[6] == 1 && f->data[7] == 0);
		}
		if (id == VID_STRG2_SOC)
		{
			memcpy(&v, &f->data[2], 4);
			CHECK(v == 81.5f && f->data[6] == 0);
		}
	}
	CHECK(bad == 0);           /* never faster than the period */
	CHECK(maxPerTick == 1);    /* max 1 CAN frame per 10 ms */
	frames = 0;
	for (k = 0; k < VARS_COUNT; k++)
	{
		const sVarMeta* m = &VarMeta[k];
		if (m->Producer == node && (m->Flags & VMF_ON_CAN))
		{
			int exp = 60000 / m->PeriodMs;
			if (!(perId[m->Id] >= exp - 1 && perId[m->Id] <= exp)) printf("  id %u: %d frames, expected ~%d\n", m->Id, perId[m->Id], exp);
			CHECK(perId[m->Id] >= exp - 1 && perId[m->Id] <= exp);
			frames += perId[m->Id];
		}
	}
	CHECK(frames == gTxN);
	CHECK(gHookN == gTxN && gHookCobid == 0x180u + node);   /* every CAN frame also to the hook */
	printf("tx: %d frames in 60 s from node %u\n", gTxN, node);

	/* VARS2_SendLegacy(): own variable now, other node's ignored */
	gTxN = 0;
	VARS2_SendLegacy(VID_AXPERT_BAT_VOLTAGE_V);
	VARS2_SendLegacy(VID_BAT_SOC);
	CHECK(gTxN == 1 && (gTx[0].data[0] | (gTx[0].data[1] << 8)) == VID_AXPERT_BAT_VOLTAGE_V);
	VARS2_SetFrameHook(NULL);
}

static void test_local(void)
{
	int counts[VAR_MAX_ID + 1];
	int i, bad = 0;
	uint32_t t;
	uint16_t k;

	Reset(NODEID_ELECON);
	VARS2_UpdateLocal_10ms(NULL);   /* no callback: nothing */
	for (t = 0; t < 30000; t += 10)
	{
		gTick += 10;
		VARS2_UpdateLocal_10ms(LocalCb);
	}
	memset(counts, 0, sizeof counts);
	for (i = 0; i < gLocN; i++)
	{
		const sVarMeta* m = Meta(gLocIds[i]);
		if (m->Producer != NODEID_ELECON || (m->Flags & VMF_ON_CAN)) bad++;
		counts[gLocIds[i]]++;
	}
	CHECK(bad == 0);
	CHECK(gTxN == 0);   /* nothing on CAN */
	for (k = 0; k < VARS_COUNT; k++)
	{
		const sVarMeta* m = &VarMeta[k];
		if (m->Producer == NODEID_ELECON && !(m->Flags & VMF_ON_CAN))
		{
			int exp = 30000 / m->PeriodMs;
			CHECK(counts[m->Id] >= exp - 1 && counts[m->Id] <= exp);
		}
	}
	printf("local: %d tunnel frames in 30 s\n", gLocN);
}

static void test_legacy_roundtrip(void)
{
	uint16_t k;
	int32_t raw;
	int bad = 0, n = 0;

	for (k = 0; k < VARS_COUNT; k++)
	{
		const sVarMeta* m = &VarMeta[k];
		Reset(m->Producer);
		for (raw = -32768; raw <= 32767; raw++)
		{
			uint16_t inv = 0;
			uint8_t valid = (uint8_t)(raw & 1);
			int16_t back;
			if (VARS2_SetLegacy(m->LegacyId, (int16_t)raw, valid) != 1) bad++;
			back = VARS2_GetLegacy(m->LegacyId, &inv, 0x01);
			if (back != (int16_t)raw || (inv != 0) != (valid == 0)) bad++;
			n++;
		}
	}
	printf("legacy round trip: %d values, %d bad\n", n, bad);
	CHECK(bad == 0);

	/* not own / unknown */
	Reset(NODEID_TECHM);
	CHECK(VARS2_SetLegacy(VID_BAT_VOLTAGE_V, 520, 1) == 0);   /* TECHM APP.c init write: ignored */
	{
		uint16_t inv = 0;
		CHECK(VARS2_GetLegacy(VID_BAT_VOLTAGE_V, &inv, 0x01) == 0 && inv == 0x01);
		inv = 0;
		CHECK(VARS2_GetLegacy(2, &inv, 0x01) == 0 && inv == 0x01);
		CHECK(VARS2_GetLegacy(2, NULL, 0x01) == 0);
	}
}

int main(void)
{
	test_init();
	test_rx();
	test_timeout();
	test_group_valid();
	test_set_by_id();
	test_tx();
	test_local();
	test_legacy_roundtrip();
	printf("%d checks, %d failed\n", checks, fails);
	if (fails) { printf("TEST_VARS2 FAILED\n"); return 1; }
	printf("TEST_VARS2 PASSED\n");
	return 0;
}
