/*
 * test_com.c - PC unit test of UHA_COMMON/Src/COM.c (VARS2 direct access).
 *
 * Real COM.c, VARS2.c and uha_vars_gen.c; MCAN and HAL_GetTick() are mocked.
 *  - streaming, for every producing node: COM_Update_10ms() sends all own on_can variables
 *    by their YAML period, only 0x180+node, values and
 *    validity from Vars2, max 1 frame per 10 ms;
 *  - RX: a received old 0x110+node frame changes nothing in Vars2 and is handed to the
 *    application like any other non-VARS2 frame; a VARS2 frame is consumed and stored.
 * Run via tests/test_com.py.
 */

#include <stdio.h>
#include <string.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "MCAN.h"
#include "COM.h"
#include "CMD.h"
/* keep equal to VARS2_CAN_MAX_PER_CALL in Src/VARS2.c */
#define VARS2_TEST_CAN_MAX  2

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
static void RunStream(uint32_t ms)
{
	uint32_t t;
	for (t = 0; t < ms; t += 10)
	{
		gTick += 10;
		COM_Update_10ms();
	}
}

static void test_stream(uint8_t node)
{
	int i, k;
	int perId[VAR_MAX_ID + 1];
	uint32_t lastTick[VAR_MAX_ID + 1];
	int badInterval = 0, maxPerTick = 0, inTick = 0, nVars = 0;
	uint32_t prev = 0;

	gTick = 0;
	gTxN = 0;
	COM_Init(node);
	for (k = 0; k < VARS_COUNT; k++)
	{
		const sVarMeta* m = &VarMeta[k];
		if (m->Producer == node)
		{
			(void)VARS2_SetById((eVarId)m->Id, (float)(100 + m->Id), (m->Id % 5u) != 0u);
		}
	}
	RunStream(60000);
	memset(perId, 0, sizeof perId);
	memset(lastTick, 0, sizeof lastTick);
	for (i = 0; i < gTxN; i++)
	{
		const sTx* f = &gTx[i];
		uint16_t id = (uint16_t)(f->data[0] | (f->data[1] << 8));
		const sVarMeta* m;
		float v;
		int32_t iv;
		CHECK(f->cobid == 0x180u + node);                        /* only VARS2 */
		if (id > VAR_MAX_ID || VarIdToIndex[id] == VAR_IDX_UNKNOWN) { CHECK(0); continue; }
		m = &VarMeta[VarIdToIndex[id]];
		CHECK(m->Producer == node && (m->Flags & VMF_ON_CAN));
		if (m->Type == VT_FLOAT)
		{
			memcpy(&v, &f->data[2], 4);
			CHECK(v == (float)(100 + id));
		}
		else
		{
			memcpy(&iv, &f->data[2], 4);
			CHECK(iv == 100 + id);
		}
		CHECK(((f->data[6] & 1u) != 0) == ((id % 5u) != 0u));
		if (perId[id] > 0 && (f->tick - lastTick[id]) < m->PeriodMs) badInterval++;
		perId[id]++;
		lastTick[id] = f->tick;
		if (f->tick == prev) inTick++; else inTick = 1;
		if (inTick > maxPerTick) maxPerTick = inTick;
		prev = f->tick;
	}
	CHECK(badInterval == 0);   /* never faster than the period */
	CHECK(maxPerTick <= VARS2_TEST_CAN_MAX);    /* = VARS2_CAN_MAX_PER_CALL frames per 10 ms */
	for (k = 0; k < VARS_COUNT; k++)
	{
		const sVarMeta* m = &VarMeta[k];
		if (m->Producer == node && (m->Flags & VMF_ON_CAN))
		{
			int exp = 60000 / m->PeriodMs;
			nVars++;
			if (!(perId[m->Id] >= exp - 1 && perId[m->Id] <= exp)) printf("  id %u: %d frames, expected ~%d\n", m->Id, perId[m->Id], exp);
			CHECK(perId[m->Id] >= exp - 1 && perId[m->Id] <= exp);
		}
	}
	printf("node %u: %d on_can variables, %d frames in 60 s\n", node, nVars, gTxN);
}

/* ---- RX ---- */
#define OLD_VAR_FRAME 0x110u   /* old variable frame 0x110+node (no longer used) */
static void test_rx(void)
{
	static sVars before;
	s_CanRxMsg msg;
	uint8_t oldFrame[8] = { 0, VID_BAT_SOC, 0, 55, 0, 1, 0, 0 };   /* old format: id, value 55, valid */
	uint8_t oldOwn[8]   = { 0, VID_TEMP_TECHM_BOARD_C, 0x01, 0x00, 0, 1, 0, 0 };
	uint8_t newFrame[8] = { VID_BAT_SOC, 0, 0, 0, 0x5C, 0x42, 1, 0 };   /* VARS2: id 10, 55.0f, valid */

	gTick = 1000;
	gRxHead = gRxTail = 0;
	COM_Init(NODEID_TECHM);
	(void)VARS2_SetById(VID_TEMP_TECHM_BOARD_C, 25.0f, 1);   /* own variable of the receiver */
	memcpy(&before, &Vars2, sizeof Vars2);

	PushRx(OLD_VAR_FRAME + NODEID_ELECON, oldFrame);
	PushRx(OLD_VAR_FRAME + NODEID_ELECON, oldOwn);
	CHECK(COM_GetRxMessage(&msg) == 1 && msg.header.StdId == OLD_VAR_FRAME + NODEID_ELECON);
	CHECK(COM_GetRxMessage(&msg) == 1);
	CHECK(memcmp(&before, &Vars2, sizeof Vars2) == 0);   /* old frame: nothing changed */

	PushRx(0x180u + NODEID_ELECON, newFrame);
	CHECK(COM_GetRxMessage(&msg) == 0);                  /* VARS2 frame consumed */
	CHECK(Vars2.GlobalEnergy.Battery.Soc_pct.value == 55.0f && Vars2.GlobalEnergy.Battery.Soc_pct.valid == 1);
	CHECK(Vars2.Temperatures.Techm.Board_C.value == 25.0f && Vars2.Temperatures.Techm.Board_C.valid == 1);
}

/* ---- commands (CMD_spec.md sec.4): 0x200 + own node -> CMD_ProcessFrame() -> APP_OnCommand() ---- */
static int gCmdCalls;
static eCmdId gCmdId;
static float gCmdValue;
void APP_OnCommand(eCmdId id, float value) { gCmdCalls++; gCmdId = id; gCmdValue = value; }

static void PushRxDlc(uint16_t stdId, const uint8_t* data, uint8_t dlc, uint8_t rtr)
{
	PushRx(stdId, data);
	gRxQueue[gRxTail - 1].header.DLC = dlc;
	gRxQueue[gRxTail - 1].header.RTR = rtr;
}

static void test_cmd_rx(void)
{
	s_CanRxMsg msg;
	uint8_t on[8]      = { CMDID_LIGHT_STAIRS & 0xFF, CMDID_LIGHT_STAIRS >> 8, 100, 0, 0, 0, 7, 0 };
	uint8_t big[8]     = { CMDID_LIGHT_KITCHEN & 0xFF, CMDID_LIGHT_KITCHEN >> 8, 0x00, 0x00, 0x00, 0x80, 8, 0 };  /* 2^31 */
	uint8_t unknown[8] = { 0xE7, 0x03, 1, 0, 0, 0, 9, 0 };   /* id 999 */
	uint8_t btn[8]     = { 0, 5, 0, 1, 0, 0, 0, 0 };

	gRxHead = gRxTail = 0;
	COM_Init(NODEID_IOBOARD_U);

	gCmdCalls = 0;
	PushRx(0x200u + NODEID_IOBOARD_U, on);
	CHECK(COM_GetRxMessage(&msg) == 0);                       /* consumed */
	CHECK(gCmdCalls == 1 && gCmdId == CMDID_LIGHT_STAIRS && gCmdValue == 100.0f);

	gRxHead = gRxTail = 0; gCmdCalls = 0;
	PushRx(0x200u + NODEID_IOBOARD_U, big);                    /* uint32 is read unsigned */
	CHECK(COM_GetRxMessage(&msg) == 0 && gCmdCalls == 1 && gCmdValue == 2147483648.0f);

	gRxHead = gRxTail = 0; gCmdCalls = 0;
	PushRx(0x200u + NODEID_TECHM, on);                         /* other node's command: dropped */
	PushRx(0x200u + NODEID_IOBOARD_U, unknown);                /* unknown id: ignored */
	PushRxDlc(0x200u + NODEID_IOBOARD_U, on, 7, CAN_RTR_DATA); /* DLC 7: ignored */
	PushRxDlc(0x200u + NODEID_IOBOARD_U, on, 8, 1);            /* RTR: ignored */
	PushRx(0x100u + NODEID_IOBOARD_D, btn);                    /* normal frame after them */
	CHECK(COM_GetRxMessage(&msg) == 1 && msg.header.StdId == 0x100u + NODEID_IOBOARD_D);
	CHECK(gCmdCalls == 0);
	CHECK(COM_GetRxMessage(&msg) == 0);

	/* the right CAN id but the command's target is another node: ignored by CMD.c */
	gRxHead = gRxTail = 0; gCmdCalls = 0;
	COM_Init(NODEID_TECHM);
	PushRx(0x200u + NODEID_TECHM, on);
	CHECK(COM_GetRxMessage(&msg) == 0 && gCmdCalls == 0);
	CHECK(CMD_ProcessFrame(on) == 0);
	COM_Init(NODEID_IOBOARD_U);
	CHECK(CMD_ProcessFrame(on) == 1 && gCmdCalls == 1);

	/* clamp helper */
	CHECK(CMD_Clamp(150.0f, 0.0f, 100.0f) == 100.0f);
	CHECK(CMD_Clamp(-1.0f, 0.0f, 100.0f) == 0.0f);
	CHECK(CMD_Clamp(42.0f, 0.0f, 100.0f) == 42.0f);
	{
		volatile float zero = 0.0f;
		CHECK(CMD_Clamp(zero / zero, 0.0f, 100.0f) == 0.0f);   /* NaN -> min */
	}
}

int main(void)
{
	static const uint8_t nodes[] = { NODEID_IOBOARD_D, NODEID_IOBOARD_U, NODEID_TECHM, NODEID_ELECON,
	                                 NODEID_RECU, NODEID_ELECON_D };
	unsigned k;
	for (k = 0; k < sizeof nodes; k++)
	{
		test_stream(nodes[k]);
	}
	test_rx();
	test_cmd_rx();
	printf("%d checks, %d failed\n", checks, fails);
	if (fails) { printf("TEST_COM FAILED\n"); return 1; }
	printf("TEST_COM PASSED\n");
	return 0;
}
