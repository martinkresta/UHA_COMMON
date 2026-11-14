/*
 * test_compat.c - old vs new: the same driver is built twice by test_compat.py,
 * once with the VARS/VARS2/COM sources of a reference commit (before direct
 * access: Vars[] + COM stream list + VARS2_SendLegacy) and once with the current
 * sources (Vars2 + YAML-driven TX + VARS.c wrappers). It uses only the API that
 * both have (VAR_SetVariable/GetVariable, COM_AddStreamedVariable, COM_Update_10ms,
 * COM_GetRxMessage, VARS2_UpdateLocal_10ms) and prints, for every producing node:
 *
 *   C <node> <phase> <id> <8 data bytes>   last CAN frame 0x180+node of the variable in the phase
 *   L <node> <phase> <id> <8 data bytes>   last tunnel frame (on_can: false variables)
 *   O <node> <phase> <id> <value> <inv>    own variable read back with VAR_GetVariable()
 *   G <node> <phase> <id> <value> <inv>    received variable read with VAR_GetVariable()
 *   T <node> <id> <value> <inv>            received variable after its timeout
 *   U <node> <id> <value> <inv>            never received variable
 *
 * test_compat.py runs it once per producing node (separate processes) and compares
 * the two outputs line by line (must be identical).
 * Inputs cover every legacy_scale, negative values, int16 limits and every type.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "VARS.h"
#include "MCAN.h"
#include "COM.h"

/* ---- mocks ---- */
static uint32_t gTick;
uint32_t HAL_GetTick(void) { return gTick; }

static uint8_t gNode;
static uint8_t gPhase;
static uint32_t gPhaseStart;
static uint8_t gLastCan[VAR_MAX_ID + 1][8];
static uint8_t gHasCan[VAR_MAX_ID + 1];
static uint8_t gLastLoc[VAR_MAX_ID + 1][8];
static uint8_t gHasLoc[VAR_MAX_ID + 1];
static int gBadCobid;

uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	uint16_t id = (uint16_t)(data[0] | (data[1] << 8));
	if (cobid != 0x180u + gNode || dlc != 8 || id > VAR_MAX_ID)
	{
		gBadCobid++;
		return 1;
	}
	if (gTick > gPhaseStart)
	{
		memcpy(gLastCan[id], data, 8);
		gHasCan[id] = 1;
	}
	return 1;
}

static void LocalCb(uint16_t cobid, const uint8_t* data)
{
	uint16_t id = (uint16_t)(data[0] | (data[1] << 8));
	if (cobid != 0x180u + gNode || id > VAR_MAX_ID)
	{
		gBadCobid++;
		return;
	}
	if (gTick > gPhaseStart)
	{
		memcpy(gLastLoc[id], data, 8);
		gHasLoc[id] = 1;
	}
}

static s_CanRxMsg gRxQueue[4];
static int gRxHead, gRxTail;
uint8_t MCAN_GetRxMessage(s_CanRxMsg* msg)
{
	if (gRxHead == gRxTail) return 0;
	*msg = gRxQueue[gRxHead++];
	if (gRxHead == gRxTail) gRxHead = gRxTail = 0;
	return 1;
}
static s_CanNode gNodes[MAX_NUM_OF_NODES];
s_CanNode* MCAN_GetNodesPt(void) { return gNodes; }
static s_McanDiag gDiag;
const s_McanDiag* MCAN_GetDiag(void) { return &gDiag; }
uint32_t MCAN_GetEsr(void) { return 0; }
uint32_t MCAN_GetTsr(void) { return 0; }

/* ---- inputs ---- */
static const int16_t RAWS[] = { 0, 1, -1, 7, -7, 55, -55, 123, -123, 999, -999, 4321, -4321,
                                12345, -12345, 32767, -32768, 250, 520, -300 };
#define NRAWS (int)(sizeof RAWS / sizeof RAWS[0])

static int16_t RawOf(uint16_t id, uint8_t phase) { return RAWS[(id * 7u + phase * 3u) % NRAWS]; }
static uint8_t ValidOf(uint16_t id, uint8_t phase) { return (uint8_t)(((id + phase) % 3u) != 0u); }

/* received values: physical floats incl. ones not representable in the legacy scale */
static const float RXF[] = { 0.0f, 1.0f, -1.0f, 53.2f, -12.75f, 3.14159f, 0.05f, -0.05f, 1234.5f,
                             -3276.8f, 3276.7f, 32.767f, 655.36f, 99999.0f, -99999.0f, 0.1f, 0.123456f };
#define NRXF (int)(sizeof RXF / sizeof RXF[0])
static const int32_t RXI[] = { 0, 1, -1, 2, 11, 255, 32767, -32768, 40000, -40000, 0x7F };
#define NRXI (int)(sizeof RXI / sizeof RXI[0])

static void PushFrame(uint16_t cobid, uint16_t id, const void* value4, uint8_t valid)
{
	s_CanRxMsg m;
	memset(&m, 0, sizeof m);
	m.header.StdId = cobid;
	m.header.DLC = 8;
	m.header.RTR = CAN_RTR_DATA;
	m.data[0] = (uint8_t)(id & 0xFFu);
	m.data[1] = (uint8_t)(id >> 8);
	memcpy(&m.data[2], value4, 4);
	m.data[6] = valid ? 1u : 0u;
	gRxQueue[gRxTail++] = m;
}

static void Run(uint32_t ms)
{
	uint32_t t;
	for (t = 0; t < ms; t += 10)
	{
		gTick += 10;
		COM_Update_10ms();
		VARS2_UpdateLocal_10ms(LocalCb);
	}
}

static void PrintFrame(char kind, uint16_t id, const uint8_t* d)
{
	printf("%c %u %u %u %02X%02X%02X%02X%02X%02X%02X%02X\n", kind, gNode, gPhase, id,
	       d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
}

static void RunNode(uint8_t node)
{
	uint16_t i;
	s_CanRxMsg msg;

	gNode = node;
	gTick = 1000;
	gRxHead = gRxTail = 0;
	VAR_Init();
	COM_Init(node);
	VAR_EnableVars2Read();

	/* like the APP.c files: stream own on_can variables with their period */
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer == node && (m->Flags & VMF_ON_CAN))
		{
			COM_AddStreamedVariable(m->LegacyId, m->PeriodMs);
		}
	}

	/* never received */
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer != node)
		{
			uint16_t inv = 0;
			int16_t v = VAR_GetVariable(m->LegacyId, &inv);
			printf("U %u %u %d %u\n", node, m->Id, v, inv);
		}
	}

	for (gPhase = 0; gPhase < 3; gPhase++)
	{
		memset(gHasCan, 0, sizeof gHasCan);
		memset(gHasLoc, 0, sizeof gHasLoc);
		gPhaseStart = gTick;

		/* own variables: producer writes */
		for (i = 0; i < VARS_COUNT; i++)
		{
			const sVarMeta* m = &VarMeta[i];
			if (m->Producer == node)
			{
				VAR_SetVariable(m->LegacyId, RawOf(m->Id, gPhase), ValidOf(m->Id, gPhase));
			}
		}

		/* received variables: one frame each, from the producer node */
		for (i = 0; i < VARS_COUNT; i++)
		{
			const sVarMeta* m = &VarMeta[i];
			if (m->Producer != node)
			{
				if (m->Type == VT_FLOAT)
				{
					float f = RXF[(m->Id + gPhase * 5u) % NRXF];
					PushFrame((uint16_t)(0x180u + m->Producer), m->Id, &f, ValidOf(m->Id, (uint8_t)(gPhase + 1)));
				}
				else
				{
					int32_t v = RXI[(m->Id + gPhase * 5u) % NRXI];
					PushFrame((uint16_t)(0x180u + m->Producer), m->Id, &v, ValidOf(m->Id, (uint8_t)(gPhase + 1)));
				}
				while (COM_GetRxMessage(&msg)) { }
				{
					uint16_t inv = 0;
					int16_t v = VAR_GetVariable(m->LegacyId, &inv);
					printf("G %u %u %u %d %u\n", node, gPhase, m->Id, v, inv);
				}
			}
		}

		Run(12000);   /* > max period (5000) + pacing of up to ~80 own CAN variables */

		for (i = 0; i < VARS_COUNT; i++)
		{
			const sVarMeta* m = &VarMeta[i];
			uint16_t inv = 0;
			int16_t v;
			if (m->Producer == node)
			{
				if (m->Flags & VMF_ON_CAN)
				{
					if (gHasCan[m->Id]) PrintFrame('C', m->Id, gLastCan[m->Id]);
					else printf("C %u %u %u none\n", node, gPhase, m->Id);
				}
				else if (node == NODEID_ELECON)
				{
					if (gHasLoc[m->Id]) PrintFrame('L', m->Id, gLastLoc[m->Id]);
					else printf("L %u %u %u none\n", node, gPhase, m->Id);
				}
				v = VAR_GetVariable(m->LegacyId, &inv);
				printf("O %u %u %u %d %u\n", node, gPhase, m->Id, v, inv);
			}
		}
	}

	/* the last phase's received values: read before and after their timeout */
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer != node)
		{
			uint16_t inv = 0;
			int16_t v;
			if (m->Type == VT_FLOAT)
			{
				float f = RXF[(m->Id + 7u) % NRXF];
				PushFrame((uint16_t)(0x180u + m->Producer), m->Id, &f, 1);
			}
			else
			{
				int32_t iv = RXI[(m->Id + 7u) % NRXI];
				PushFrame((uint16_t)(0x180u + m->Producer), m->Id, &iv, 1);
			}
			while (COM_GetRxMessage(&msg)) { }
			v = VAR_GetVariable(m->LegacyId, &inv);
			printf("G %u 9 %u %d %u\n", node, m->Id, v, inv);
		}
	}
	Run(16000);   /* > max timeout (15000) */
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer != node)
		{
			uint16_t inv = 0;
			int16_t v = VAR_GetVariable(m->LegacyId, &inv);
			printf("T %u %u %d %u\n", node, m->Id, v, inv);
		}
	}
}

int main(int argc, char** argv)
{
	/* one node per process: the old COM stream list cannot be cleared */
	int node = (argc > 1) ? atoi(argv[1]) : 0;
	if (node <= 0 || node > 15)
	{
		printf("usage: test_compat <node id>\n");
		return 2;
	}
	RunNode((uint8_t)node);
	printf("X bad-cobid %d\n", gBadCobid);
	return 0;
}
