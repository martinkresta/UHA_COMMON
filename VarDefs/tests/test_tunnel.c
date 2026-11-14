/*
 * test_tunnel.c - PC unit test of the ELECON VARS2 tunnel (step 2):
 * ASW_ELECON/Core/Src/RPISERP.c (TX FIFO, chained TX) and SCOM.c (hook,
 * local publishing incl. NETWORK_STATUS, no old 0x221 traffic) with a simulated UART/DMA.
 * RPISERP.c and SCOM.c are #included so their static functions are covered; VARS2.c and
 * the VARS.c wrappers (direct access) are linked.
 * Run via tests/test_tunnel.py.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "VARS.h"
#include "MCAN.h"
#include "COM.h"
#include "RTC.h"
#include "APP.h"

/* copies of the ELECON sources made by test_tunnel.py (so that the stub main.h wins) */
#include "RPISERP.c"
#include "SCOM.c"

static int fails = 0;
static int checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* ---- mocks ---- */
static uint32_t gTick;
uint32_t HAL_GetTick(void) { return gTick; }
DMA_HandleTypeDef hdma_usart1_rx, hdma_usart1_tx;
DMA_TypeDef gDma1;
int gIrqDisabled;
static USART_TypeDef gUsart;
static DMA_Channel_TypeDef gDmaTx, gDmaRx;
static UART_HandleTypeDef gUart;

/* VAR_SetVariable()/VAR_GetVariable(): the real VARS.c wrappers over Vars2 are linked */
static int gCanSent;
uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc) { (void)cobid; (void)data; (void)dlc; gCanSent++; return 1; }
void COM_SendMessage(uint16_t cobid, uint8_t* data, uint8_t dlc) { (void)cobid; (void)data; (void)dlc; }
int16_t COM_GetNetworkStatus(void) { return 0x1F; }
void RTC_SetUnixTime(uint32_t t) { (void)t; }
void APP_RpiHeartbeat(void) {}

/* ---- simulated serial line: records the frames the DMA/UART "sent" ---- */
#define MAXCAP 20000
static uint8_t gCap[MAXCAP][RSP_PACKET_SIZE];
static int gCapLen[MAXCAP];
static int gCapN;
static int gBudget;   /* bytes the line can still carry */

static void LinePump(void)
{
	while ((gDmaTx.CCR & DMA_CCR_EN) && gBudget >= (int)gDmaTx.CNDTR)
	{
		int n = (int)gDmaTx.CNDTR;
		if (gCapN < MAXCAP) { memcpy(gCap[gCapN], mTxData, n); gCapLen[gCapN] = n; gCapN++; }
		gBudget -= n;
		DMA1_Channel4_IRQHandler();      /* DMA TC (disables the channel) */
		gUsart.ISR |= USART_ISR_TC;
		USART1_IRQHandler();             /* UART TC: chains the next frame */
		gUsart.ISR &= ~USART_ISR_TC;
	}
}

static void SetupLink(void)
{
	memset(&gUsart, 0, sizeof(gUsart)); memset(&gDmaTx, 0, sizeof(gDmaTx)); memset(&gDmaRx, 0, sizeof(gDmaRx));
	gUart.Instance = &gUsart;
	hdma_usart1_tx.Instance = &gDmaTx;
	hdma_usart1_rx.Instance = &gDmaRx;
	gCapN = 0; gBudget = 0; gIrqDisabled = 0; gRspTxOverflow = 0; gRspTxFrames = 0;
	SCOM_Init(&gUart);   /* resets the TX FIFO via RSP_Init, installs the hook */
}

/* decoded packet */
typedef struct { int ok; uint16_t id; uint8_t len; uint8_t data[8]; } sPkt;
static sPkt Decode(int i)
{
	sPkt p; uint8_t sum = 0; int k, n = gCapLen[i];
	memset(&p, 0, sizeof(p));
	if (n < 5 || gCap[i][0] != 0x7F || gCap[i][1] != 0xAA) return p;
	if (gCap[i][2] + 4 != n) return p;
	for (k = 0; k < n - 1; k++) sum = (uint8_t)(sum + gCap[i][k]);
	if (sum != gCap[i][n - 1]) return p;
	p.ok = 1; p.len = (uint8_t)(gCap[i][2] - 2);
	p.id = (uint16_t)((gCap[i][3] << 8) | gCap[i][4]);
	memcpy(p.data, &gCap[i][5], p.len);
	return p;
}

static void Tick10(void)
{
	gTick += 10;
	gBudget += 115;   /* 115200 Bd / 10 bit per byte = 11520 B/s */
	SCOM_Update_10ms();
	LinePump();
}

/* ---- tests ---- */
static void test_rx_tunnel_and_reject(void)
{
	uint8_t f[8] = { 11, 0, 0x00, 0x00, 0x50, 0x42, 0x01, 0 };   /* id 11 = 52.0 V */
	uint8_t other[8] = { 0xFF, 0xFF, 1, 2, 3, 4, 5, 6 };
	sPkt p;

	VARS2_Init(NODEID_ELECON);
	SetupLink();
	gBudget = 100000;

	CHECK(VARS2_ProcessRxFrame(0x187, 8, 0, f) == 1);
	CHECK(VARS2_ProcessRxFrame(0x187, 7, 0, other) == 0);   /* bad DLC: dropped */
	CHECK(VARS2_ProcessRxFrame(0x187, 8, 1, other) == 0);   /* RTR: dropped */
	LinePump();
	CHECK(gCapN == 1);
	p = Decode(0);
	CHECK(p.ok && p.id == 0x187 && p.len == 8 && memcmp(p.data, f, 8) == 0);
	CHECK(gCapLen[0] == 14);

	/* unknown VARS2 id is still tunnelled unchanged */
	gCapN = 0;
	VARS2_ProcessRxFrame(0x182, 8, 0, other);
	LinePump();
	CHECK(gCapN == 1);
	p = Decode(0);
	CHECK(p.ok && p.id == 0x182 && memcmp(p.data, other, 8) == 0);
}

static void test_own_tx_tunnel(void)
{
	sPkt p;
	VARS2_Init(NODEID_ELECON);
	SetupLink();
	gBudget = 100000; gCanSent = 0;
	VAR_SetVariable(VAR_BAT_VOLTAGE_V10, 532, 1);
	VARS2_SendLegacy(VAR_BAT_VOLTAGE_V10);
	LinePump();
	CHECK(gCanSent == 1);
	CHECK(gCapN == 1);
	p = Decode(0);
	CHECK(p.ok && p.id == 0x184 && p.data[0] == 11 && p.data[1] == 0 && p.data[6] == 1);
}

static void test_local_and_no_old_traffic(void)
{
	int t, i;
	int cnt184 = 0, cnt221 = 0, other = 0, bad = 0, nsBad = 0;
	static int perId[VAR_MAX_ID + 1];

	VARS2_Init(NODEID_ELECON);
	SetupLink();
	for (i = 0; i < VARS_COUNT; i++)   /* own variables written by the (unconverted) producers */
	{
		if (VarMeta[i].Producer == NODEID_ELECON && VarMeta[i].Id != VID_NETWORK_STATUS)
		{
			VAR_SetVariable(VarMeta[i].LegacyId, (int16_t)(VarMeta[i].Id + 1), 1);
		}
	}
	/* NETWORK_STATUS: set by SCOM every 5 s from COM_GetNetworkStatus() (mock: 0x1F) */
	memset(perId, 0, sizeof(perId));
	gCanSent = 0; gTick = 1000000;
	for (t = 0; t < 6000; t++)   /* 60 s */
	{
		Tick10();
	}
	gBudget = 100000; LinePump();   /* drain */

	for (i = 0; i < gCapN; i++)
	{
		sPkt p = Decode(i);
		if (!p.ok) { bad++; continue; }
		if (p.id == 0x184)
		{
			uint16_t id = (uint16_t)(p.data[0] | (p.data[1] << 8));
			cnt184++;
			if (id <= VAR_MAX_ID) perId[id]++;
			if (id == VID_NETWORK_STATUS)
			{
				uint32_t v = (uint32_t)p.data[2] | ((uint32_t)p.data[3] << 8) | ((uint32_t)p.data[4] << 16) | ((uint32_t)p.data[5] << 24);
				if (!(v == 0x1F && p.data[6] == 1) && !(perId[id] == 1 && v == 0 && p.data[6] == 0)) nsBad++;   /* first one may precede the first 5 s set: 0, invalid */
			}
		}
		else if (p.id == 0x221) cnt221++;
		else other++;
	}
	printf("60 s: %d frames (0x184: %d, 0x221: %d, other %d, corrupt %d), overflow %u\n",
	       gCapN, cnt184, cnt221, other, bad, (unsigned)gRspTxOverflow);
	CHECK(bad == 0);
	CHECK(other == 0);
	CHECK(gCanSent == 0);
	CHECK(gRspTxOverflow == 0);
	CHECK(cnt184 > 2300 && cnt184 < 2700);   /* 108 local vars over 1..5 s periods, ~42/s */
	CHECK(cnt221 == 0);          /* old PC scan list removed (VARS2 step 6b part B) */
	/* NETWORK_STATUS: every 5 s on 0x184 only (not on CAN), uint32 0x1F, valid */
	printf("NETWORK_STATUS: %d frames on 0x184\n", perId[VID_NETWORK_STATUS]);
	CHECK(perId[VID_NETWORK_STATUS] >= 11 && perId[VID_NETWORK_STATUS] <= 13);
	CHECK(nsBad == 0);
	for (i = 0; i < VARS_COUNT; i++)
	{
		const sVarMeta* m = &VarMeta[i];
		if (m->Producer == NODEID_ELECON && !(m->Flags & VMF_ON_CAN) && m->PeriodMs)
		{
			int e = 60000 / m->PeriodMs;
			CHECK(perId[m->Id] >= e - 1 && perId[m->Id] <= e + 1);
		}
	}
}

static void test_fifo_overflow_and_order(void)
{
	int i, accepted;
	sPkt p;
	uint8_t d[8];

	VARS2_Init(NODEID_ELECON);
	SetupLink();   /* line blocked: budget 0 */
	for (i = 0; i < 100; i++)
	{
		memset(d, 0, 8); d[0] = 11; d[2] = (uint8_t)i;
		VARS2_ProcessRxFrame(0x181, 8, 0, d);
	}
	/* 1 frame is already in the UART (taken from the FIFO at once), 64 wait in the FIFO */
	accepted = 1 + RSP_TX_FIFO_SIZE;
	CHECK(RSP_GetTxOverflowCount() == (uint32_t)(100 - accepted));
	gBudget = 100000; LinePump();
	CHECK(gCapN == accepted);
	for (i = 0; i < gCapN; i++)
	{
		p = Decode(i);
		CHECK(p.ok && p.data[2] == (uint8_t)i);   /* FIFO order, the newest were dropped */
	}
	CHECK(gIrqDisabled == 0);   /* critical sections are balanced */
}

static void test_throughput(void)
{
	int t, k;
	uint8_t d[8];
	VARS2_Init(NODEID_ELECON);
	SetupLink();
	gTick = 5000;
	/* 500 frames/s of 14 B offered (5 per 10 ms) = 7000 B/s, link 11520 B/s */
	for (t = 0; t < 1000; t++)
	{
		for (k = 0; k < 5; k++) { memset(d, 0, 8); d[0] = 11; VARS2_ProcessRxFrame(0x182, 8, 0, d); }
		gTick += 10; gBudget += 115; RSP_TransmitFromFifo(); LinePump();
	}
	printf("offered 5000 frames in 10 s, sent %d, overflow %u\n", gCapN, (unsigned)gRspTxOverflow);
	CHECK(gRspTxOverflow == 0);
	CHECK(gCapN >= 4990);   /* the rest is still in the FIFO / on the line */
}

int main(void)
{
	/* the hook is installed by SCOM_Init() */
	test_rx_tunnel_and_reject();
	test_own_tx_tunnel();
	test_local_and_no_old_traffic();
	test_fifo_overflow_and_order();
	test_throughput();
	printf("%d checks, %d failed\n", checks, fails);
	printf(fails ? "TEST_TUNNEL FAILED\n" : "TEST_TUNNEL PASSED\n");
	return fails ? 1 : 0;
}
