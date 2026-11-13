/*
 * test_com.c - PC unit test of UHA_COMMON/Src/COM.c after VARS2 step 6b part A
 * (old variable frame 0x110+node removed).
 *
 * Real COM.c, VARS.c, VARS2.c and uha_vars_gen.c; MCAN and HAL_GetTick() are mocked.
 *  - streaming: an ELECON-like stream list (all own on_can variables with their
 *    YAML period, one invalid, one id without a VARS2 row) for 60 s: only
 *    0x180+node leaves, values/validity correct, every variable at its period;
 *  - RX: a received old 0x110+node frame changes neither Vars[] nor Vars2 nor
 *    any validity; it is handed to the application like any other non-VARS2 frame.
 *
 * "test_com --trace" prints the CAN TX traces (normal stream 60 s, overload stream 10 s)
 * and the RX results instead of checking;
 * test_com.py --ref <old COM.c> builds the same test with the old COM.c and compares
 * the 0x18x traces (stream timing unchanged). Run via tests/test_com.py.
 */

#include <stdio.h>
#include <string.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "VARS.h"
#include "MCAN.h"
#include "COM.h"

extern sVar Vars[NUM_OF_VARIABLES];   /* VARS.c */

static int fails = 0;
static int checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* ---- mocks ---- */
static uint32_t gTick;
uint32_t HAL_GetTick(void) { return gTick; }

#define MAXTX 20000
typedef struct { uint32_t tick; uint16_t cobid; uint8_t data[8]; } sTx;
static sTx gTx[MAXTX];
static int gTxN;
uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	if (gTxN < MAXTX)
	{
		gTx[gTxN].tick = gTick;
		gTx[gTxN].cobid = cobid;
		memset(gTx[gTxN].data, 0, 8);
		memcpy(gTx[gTxN].data, data, dlc <= 8 ? dlc : 8);
		gTxN++;
	}
	return 1;
}

static s_CanRxMsg gRxQueue[8];
static int gRxHead, gRxTail;
uint8_t MCAN_GetRxMessage(s_CanRxMsg* msg)
{
	if (gRxHead == gRxTail) return 0;
	*msg = gRxQueue[gRxHead++];
	return 1;
}
static s_CanNode gNodes[MAX_NUM_OF_NODES];
s_CanNode* MCAN_GetNodesPt(void) { return gNodes; }
static s_McanDiag gDiag;
const s_McanDiag* MCAN_GetDiag(void) { return &gDiag; }
uint32_t MCAN_GetEsr(void) { return 0; }
uint32_t MCAN_GetTsr(void) { return 0; }

static void PushRx(uint16_t stdId, const uint8_t* data)
{
	s_CanRxMsg m;
	memset(&m, 0, sizeof m);
	m.header.StdId = stdId;
	m.header.DLC = 8;
	m.header.RTR = CAN_RTR_DATA;
	memcpy(m.data, data, 8);
	gRxQueue[gRxTail++] = m;
}

/* ---- streaming ---- */
#define NODE NODEID_ELECON
#define NO_ROW_ID 2          /* valid Vars[] index without VARS2 row */
#define INVALID_ID VAR_BAT_CURRENT_A10

static uint16_t gStreamIds[STREAM_LIST_SIZE];
static uint16_t gStreamPeriod[STREAM_LIST_SIZE];
static int gStreamN;

static void SetupStream(void)
{
	uint16_t i;
	gTick = 0;
	gTxN = 0;
	VAR_Init();
	COM_Init(NODE);
	gStreamN = 0;
	for (i = 0; i < VARS_COUNT && gStreamN < STREAM_LIST_SIZE - 1; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer == NODE && (m->Flags & VMF_ON_CAN))
		{
			gStreamIds[gStreamN] = m->LegacyId;
			gStreamPeriod[gStreamN] = m->PeriodMs;
			gStreamN++;
			COM_AddStreamedVariable(m->LegacyId, m->PeriodMs);
			VAR_SetVariable(m->LegacyId, (int16_t)(100 + m->Id), m->LegacyId == INVALID_ID ? 0 : 1);
		}
	}
	gStreamIds[gStreamN] = NO_ROW_ID;
	gStreamPeriod[gStreamN] = 1000;
	gStreamN++;
	COM_AddStreamedVariable(NO_ROW_ID, 1000);
	VAR_SetVariable(NO_ROW_ID, 77, 1);
}

/* overload: all own rows (incl. not on CAN) at 100 ms, more than 1 frame per 10 ms due */
static void SetupStreamOverload(void)
{
	uint16_t i;
	gTick = 0;
	gTxN = 0;
	VAR_Init();
	COM_Init(NODE);
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer == NODE)
		{
			COM_AddStreamedVariable(m->LegacyId, 100);
			VAR_SetVariable(m->LegacyId, (int16_t)m->Id, 1);
		}
	}
	COM_AddStreamedVariable(NO_ROW_ID, 100);
}

static void RunStream(uint32_t ms)
{
	uint32_t t;
	for (t = 0; t < ms; t += 10)
	{
		gTick += 10;
		COM_Update_10ms();
	}
}

static void test_stream(void)
{
	int i, k;
	int perId[VAR_MAX_ID + 1];
	uint32_t lastTick[VAR_MAX_ID + 1];
	int badInterval = 0;

	SetupStream();
	RunStream(60000);
	memset(perId, 0, sizeof perId);
	memset(lastTick, 0, sizeof lastTick);
	printf("stream: %d variables, %d frames in 60 s\n", gStreamN, gTxN);
	CHECK(gTxN > 0);
	for (i = 0; i < gTxN; i++)
	{
		const sTx* f = &gTx[i];
		uint16_t id = (uint16_t)(f->data[0] | (f->data[1] << 8));
		CHECK(f->cobid == 0x180u + NODE);                       /* only VARS2 */
		CHECK(!(f->cobid >= CMD_VAR_VALUE && f->cobid <= CMD_VAR_VALUE + 0x0F));
		CHECK(id != NO_ROW_ID);
		if (id > VAR_MAX_ID || VarIdToIndex[id] == VAR_IDX_UNKNOWN) { CHECK(0); continue; }
		{
			const sVarMeta* m = &VarMeta[VarIdToIndex[id]];
			float v;
			CHECK(m->Producer == NODE);
			CHECK(m->Type == VT_FLOAT);
			memcpy(&v, &f->data[2], 4);
			CHECK((int)(v * m->LegacyScale + (v >= 0 ? 0.5f : -0.5f)) == 100 + id);
			CHECK(((f->data[6] & 1u) != 0) == (m->LegacyId != INVALID_ID));
			if (perId[id] > 0 && (f->tick - lastTick[id]) < m->PeriodMs) badInterval++;
			perId[id]++;
			lastTick[id] = f->tick;
		}
	}
	CHECK(badInterval == 0);   /* never faster than the period */
	for (k = 0; k < gStreamN; k++)
	{
		uint16_t id = gStreamIds[k];
		int exp = 60000 / gStreamPeriod[k];
		if (id == NO_ROW_ID) { CHECK(perId[id] == 0); continue; }
		if (!(perId[id] >= exp - 1 && perId[id] <= exp)) printf("  id %u: %d frames, expected ~%d\n", id, perId[id], exp);
		CHECK(perId[id] >= exp - 1 && perId[id] <= exp);
	}
}

/* ---- RX ---- */
static sVar gVarsBefore[NUM_OF_VARIABLES];
static sVars gVars2Before;
static uint8_t gValidBefore[VARS_COUNT];

static void Snapshot(void)
{
	uint16_t i;
	memcpy(gVarsBefore, Vars, sizeof Vars);
	memcpy(&gVars2Before, &Vars2, sizeof Vars2);
	for (i = 0; i < VARS_COUNT; i++) gValidBefore[i] = VARS2_IsValid((eVarId)VarMeta[i].Id);
}

static int Unchanged(void)
{
	uint16_t i;
	if (memcmp(gVarsBefore, Vars, sizeof Vars) != 0) return 0;
	if (memcmp(&gVars2Before, &Vars2, sizeof Vars2) != 0) return 0;
	for (i = 0; i < VARS_COUNT; i++) if (gValidBefore[i] != VARS2_IsValid((eVarId)VarMeta[i].Id)) return 0;
	return 1;
}

typedef struct { uint8_t rc1; uint32_t id1; int unchanged1; uint8_t rc2; int unchanged2; int16_t soc; uint16_t inv; } sRxResult;

static sRxResult RunRx(void)
{
	sRxResult r;
	s_CanRxMsg msg;
	uint8_t oldFrame[8] = { 0, VAR_BAT_SOC, 0, 55, 0, 1, 0, 0 };   /* old format: id, value 55, valid */
	uint8_t oldOwn[8]   = { 0, VAR_TEMP_TECHM_BOARD, 0x01, 0x00, 0, 1, 0, 0 };
	uint8_t newFrame[8] = { VAR_BAT_SOC, 0, 0, 0, 0x5C, 0x42, 1, 0 };   /* VARS2: id 10, 55.0f, valid */

	memset(&r, 0, sizeof r);
	gTick = 1000;
	gRxHead = gRxTail = 0;
	VAR_Init();
	COM_Init(NODEID_TECHM);
	VAR_EnableVars2Read();
	VAR_SetVariable(VAR_TEMP_TECHM_BOARD, 250, 1);   /* own variable of the receiver */
	Snapshot();

	PushRx(CMD_VAR_VALUE + NODEID_ELECON, oldFrame);
	PushRx(CMD_VAR_VALUE + NODEID_ELECON, oldOwn);
	r.rc1 = COM_GetRxMessage(&msg);
	r.id1 = msg.header.StdId;
	COM_GetRxMessage(&msg);
	r.unchanged1 = Unchanged();

	/* sanity: a VARS2 frame is consumed and stored */
	PushRx(0x180u + NODEID_ELECON, newFrame);
	r.rc2 = COM_GetRxMessage(&msg);
	r.unchanged2 = Unchanged();
	r.soc = VAR_GetVariable(VAR_BAT_SOC, &r.inv);
	return r;
}

static void test_rx(void)
{
	sRxResult r = RunRx();
	CHECK(r.rc1 == 1 && r.id1 == CMD_VAR_VALUE + NODEID_ELECON);   /* passed on to the application unchanged */
	CHECK(r.unchanged1);                                             /* old frame: nothing changed in COM/VARS/VARS2 */
	CHECK(r.rc2 == 0);                                               /* VARS2 frame consumed */
	CHECK(!r.unchanged2);
	CHECK(r.soc == 55 && r.inv == 0);
}

int main(int argc, char** argv)
{
	if (argc > 1 && strcmp(argv[1], "--trace") == 0)
	{
		int i;
		sRxResult r;
		SetupStream();
		RunStream(60000);
		for (i = 0; i < gTxN; i++)
		{
			const uint8_t* d = gTx[i].data;
			printf("T %lu %03X %02X%02X%02X%02X%02X%02X%02X%02X\n", (unsigned long)gTx[i].tick, gTx[i].cobid,
			       d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
		}
		SetupStreamOverload();
		RunStream(10000);
		for (i = 0; i < gTxN; i++)
		{
			const uint8_t* d = gTx[i].data;
			printf("T %lu %03X %02X%02X%02X%02X%02X%02X%02X%02X\n", (unsigned long)gTx[i].tick, gTx[i].cobid,
			       d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
		}
		r = RunRx();
		printf("R %u %03lX %d %u %d %d %u\n", r.rc1, (unsigned long)r.id1, r.unchanged1, r.rc2, r.unchanged2, r.soc, r.inv);
		return 0;
	}
	test_stream();
	test_rx();
	printf("%d checks, %d failed\n", checks, fails);
	if (fails) { printf("TEST_COM FAILED\n"); return 1; }
	printf("TEST_COM PASSED\n");
	return 0;
}
