/*
 * test_cmd_fwd.c - PC unit test of the ELECON RPi command forwarding (CMD_spec.md section 3):
 * ASW_ELECON/Core/Src/SCOM.c ForwardRpiCommand() fed through the real RPISERP.c RX path
 * (simulated UART IDLE interrupt -> RX FIFO -> RSP_GetMessage() -> SCOM ProcessMessage()).
 *  - the CAN id is always 0x200 + (data[0] & 0x0F), for all 256 values of the target byte and any other bytes
 *  - CAN data = [cmd_lo, cmd_hi, v0..v3, seq, 0], DLC 8
 *  - data length != 8 dropped (and counted, logged); len = 10 gives exactly 8 data bytes
 *  - rate limit: the 11th command within 1 s and the 21st within 60 s are dropped
 *  - target 4 (ELECON) goes to CMD_ProcessFrame() and not to CAN
 *  - no other RPISERP packet id puts any frame on CAN through MCAN_SendFrame()
 *  - drop LOG at most once per second
 * RPISERP.c and SCOM.c are #included (static functions/state reachable). Run via tests/test_cmd_fwd.py.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uha_vars_gen.h"
#include "VARS2.h"
#include "MCAN.h"
#include "COM.h"
#include "RTC.h"
#include "APP.h"
#include "LOG.h"

/* copies of the ELECON sources made by test_cmd_fwd.py (so that the stub main.h wins) */
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

typedef struct { uint16_t cobid; uint8_t dlc; uint8_t data[8]; } sCan;
#define MAXCAN 1000
static sCan gCan[MAXCAN];
static int gCanN;
uint8_t MCAN_SendFrame(uint16_t cobid, uint8_t* data, uint8_t dlc)
{
	if (gCanN < MAXCAN)
	{
		gCan[gCanN].cobid = cobid;
		gCan[gCanN].dlc = dlc;
		memcpy(gCan[gCanN].data, data, 8);
	}
	gCanN++;
	return 1;
}
static int gComSent;
void COM_SendMessage(uint16_t cobid, uint8_t* data, uint8_t dlc) { (void)cobid; (void)data; (void)dlc; gComSent++; }
int16_t COM_GetNetworkStatus(void) { return 0x1F; }
void RTC_SetUnixTime(uint32_t t) { (void)t; }
void APP_RpiHeartbeat(void) {}

static int gLocalN;
static uint8_t gLocal[8];
uint8_t CMD_ProcessFrame(const uint8_t* data) { gLocalN++; memcpy(gLocal, data, 8); return 1; }

static int gLogN;
static eLogEvent gLogEv;
static uint8_t gLog[6];
void LOG_SendEvent(eLogEvent event, const uint8_t payload[6]) { gLogN++; gLogEv = event; memcpy(gLog, payload, 6); }

/* ---- RPISERP RX: one packet through the IDLE interrupt, then one SCOM cycle ---- */
static void RxPacket(uint16_t id, const uint8_t* data, uint8_t dataLen)
{
	uint8_t len = (uint8_t)(2 + dataLen);
	uint8_t n = (uint8_t)(len + 4);
	mRxData[0] = RSP_MSG_START_B1;
	mRxData[1] = RSP_MSG_START_B2;
	mRxData[2] = len;
	mRxData[3] = (uint8_t)(id >> 8);
	mRxData[4] = (uint8_t)(id & 0xFF);
	memcpy(&mRxData[5], data, dataLen);
	mRxData[n - 1] = Checksum(mRxData, (uint8_t)(n - 1));
	gDmaRx.CNDTR = RSP_PACKET_SIZE - n;
	gUsart.ISR |= USART_ISR_IDLE;
	USART1_IRQHandler();
	gUsart.ISR &= ~USART_ISR_IDLE;
	SCOM_Update_10ms();   /* takes the packet from the RX FIFO, ProcessMessage() */
}

static void Setup(void)
{
	memset(&gUsart, 0, sizeof(gUsart)); memset(&gDmaTx, 0, sizeof(gDmaTx)); memset(&gDmaRx, 0, sizeof(gDmaRx));
	gUart.Instance = &gUsart;
	hdma_usart1_tx.Instance = &gDmaTx;
	hdma_usart1_rx.Instance = &gDmaRx;
	VARS2_Init(NODEID_ELECON);
	SCOM_Init(&gUart, &Vars2.System);
	gCanN = 0; gLocalN = 0; gLogN = 0; gComSent = 0;
	gScomCmdDropLen = 0; gScomCmdDropRate = 0; gScomCmdForwarded = 0; gScomCmdLocal = 0;
}

static void Cmd(uint8_t target, uint8_t* d)
{
	d[0] = target;
	RxPacket(CMD_RPI_COMMAND, d, 8);
}

/* ---- tests ---- */
static void test_can_id_all_targets(void)
{
	int b, k;
	uint8_t d[8];
	Setup();
	for (b = 0; b < 256; b++)
	{
		int can0 = gCanN, loc0 = gLocalN;
		gTick += 60000;   /* new 1 s and 60 s windows: no rate limit here */
		for (k = 1; k < 8; k++) d[k] = (uint8_t)(rand() & 0xFF);
		if (b & 1) { d[1] = 0xFF; d[2] = 0xFF; d[3] = 0xFF; d[4] = 0xFF; d[5] = 0xFF; d[6] = 0xFF; }
		Cmd((uint8_t)b, d);
		if ((b & 0x0F) == NODEID_ELECON)
		{
			CHECK(gCanN == can0 && gLocalN == loc0 + 1);          /* local, never on CAN */
			CHECK(memcmp(gLocal, &d[1], 7) == 0 && gLocal[7] == 0);
		}
		else
		{
			CHECK(gLocalN == loc0 && gCanN == can0 + 1);
			CHECK(gCan[can0].cobid == 0x200u + (unsigned)(b & 0x0F));
			CHECK(gCan[can0].cobid >= 0x200u && gCan[can0].cobid <= 0x20Fu);
			CHECK(gCan[can0].dlc == 8);
			CHECK(memcmp(gCan[can0].data, &d[1], 7) == 0 && gCan[can0].data[7] == 0);
		}
	}
	CHECK(gScomCmdForwarded == 240 && gScomCmdLocal == 16);
	CHECK(gScomCmdDropLen == 0 && gScomCmdDropRate == 0 && gLogN == 0);
}

static void test_length(void)
{
	uint8_t d[8] = { NODEID_IOBOARD_U, 201 & 0xFF, 0, 100, 0, 0, 0, 1 };
	uint8_t n;
	Setup();
	for (n = 1; n < 8; n++)   /* RPISERP len 3..9 = 1..7 data bytes */
	{
		gTick += 2000;
		RxPacket(CMD_RPI_COMMAND, d, n);
		CHECK(gLogN == n && gLogEv == eleLog && gLog[0] == SCOM_LOG_CMD_DROP && gLog[1] == SCOM_CMD_DROP_LENGTH
		      && gLog[2] == n && gLog[3] == n && gLog[4] == 0 && gLog[5] == 0);
	}
	CHECK(gCanN == 0 && gLocalN == 0 && gScomCmdDropLen == 7);
	/* len = 10 -> mRxLength 10 -> exactly 8 data bytes: forwarded */
	gTick += 2000;
	RxPacket(CMD_RPI_COMMAND, d, 8);
	CHECK(mRxLength == 10);
	CHECK(gCanN == 1 && gCan[0].cobid == 0x202 && gCan[0].data[0] == 201 && gCan[0].data[2] == 100 && gCan[0].data[6] == 1);
	/* a packet that does not fit (len 11) never passes the RX interrupt (rxSize must be len + 4 <= 14) */
}

static void test_rate_limit(void)
{
	uint8_t d[8] = { 0, 201 & 0xFF, 0, 100, 0, 0, 0, 0 };
	int i;
	Setup();
	gTick = 100000;
	SCOM_Init(&gUart, &Vars2.System);   /* windows start now */
	for (i = 0; i < 10; i++) { gTick += 10; Cmd(NODEID_IOBOARD_U, d); }
	CHECK(gCanN == 10);
	gTick += 10; Cmd(NODEID_IOBOARD_U, d);                  /* 11th within 1 s */
	CHECK(gCanN == 10 && gScomCmdDropRate == 1);
	CHECK(gLogN == 1 && gLog[0] == SCOM_LOG_CMD_DROP && gLog[1] == SCOM_CMD_DROP_RATE && gLog[2] == NODEID_IOBOARD_U);
	for (i = 0; i < 50; i++) { gTick += 10; Cmd(NODEID_IOBOARD_U, d); }   /* flood: dropped, one LOG per second */
	CHECK(gCanN == 10 && gScomCmdDropRate == 51);
	CHECK(gLogN == 1);
	/* target 4 counts too and is dropped as well */
	Cmd(NODEID_ELECON, d);
	CHECK(gLocalN == 0 && gScomCmdDropRate == 52);

	gTick = 100000 + 1000;                                    /* next 1 s window: 10 more = 20 in this minute */
	for (i = 0; i < 10; i++) { gTick += 10; Cmd(NODEID_IOBOARD_U, d); }
	CHECK(gCanN == 20);
	CHECK(gLogN == 1);
	gTick = 100000 + 2000;                                    /* 21st within 60 s */
	Cmd(NODEID_IOBOARD_U, d);
	CHECK(gCanN == 20 && gScomCmdDropRate == 53);
	CHECK(gLogN == 2 && gLog[3] == 0 && gLog[4] == 53 && gLog[5] == 0);   /* > 1 s after the last LOG: logged again */
	gTick = 100000 + 59990;
	Cmd(NODEID_IOBOARD_U, d);
	CHECK(gCanN == 20);
	gTick = 100000 + 60000;                                   /* new minute */
	Cmd(NODEID_IOBOARD_U, d);
	CHECK(gCanN == 21);
	CHECK(gScomCmdForwarded == 21);
}

static void test_other_ids_never_on_can(void)
{
	uint8_t d[8] = { 0x02, 0x00, 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 };
	uint32_t id;
	Setup();
	for (id = 0; id <= 0xFFFF; id++)
	{
		if (id == CMD_RPI_COMMAND) continue;
		gTick += 60000;
		RxPacket((uint16_t)id, d, 8);
	}
	CHECK(gCanN == 0 && gLocalN == 0);
	CHECK(gComSent == 1);   /* only 0x51 RTC sync, through COM (0x510 + own node), as before */
}

int main(void)
{
	test_can_id_all_targets();
	test_length();
	test_rate_limit();
	test_other_ids_never_on_can();
	printf("%d checks, %d failed\n", checks, fails);
	printf(fails ? "TEST_CMD_FWD FAILED\n" : "TEST_CMD_FWD PASSED\n");
	return fails ? 1 : 0;
}
