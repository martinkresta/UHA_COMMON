/*
 * test_mcan.c - PC unit test of the MCAN TX supervision (feature/can_tx_supervision).
 * Built twice by test_mcan.py: with the default MCAN_TX_STALL_RESET_AFTER (5) and with 0 (never reset).
 * A simulated bus completes pending mailboxes every 1 ms unless the test puts it into "bus-off".
 */
#include <stdio.h>
#include <string.h>
#include "MCAN.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int gIrqDisabled = 0;

/* ---------------- simulated hardware ---------------- */
static CAN_TypeDef gCan;
static CAN_HandleTypeDef gHcan;
static uint32_t gNow;
static int gBusOff;          /* 1: mailboxes stay pending, nothing completes */
static int gHealOnInit;      /* 1: leaving init mode clears the bus-off */
static int gSwallow;         /* 1: AddTxMessage returns OK but the frame vanishes (no mailbox used) */
static int gInitCycles;      /* INRQ 1 -> 0 transitions */
static int gResetCalls;
static uint32_t gSentFrames;

/* reports */
static int gRep[8];                  /* count per MCAN_ERR_* code */
static uint8_t gLast[8][6];          /* last payload per code */
static int gRepBeforeReset;
static int gEvRecovery, gEvReset;

static const uint32_t TME[3]  = {CAN_TSR_TME0, CAN_TSR_TME1, CAN_TSR_TME2};
static const uint32_t RQCP[3] = {CAN_TSR_RQCP0, CAN_TSR_RQCP1, CAN_TSR_RQCP2};
static const uint32_t TXOK[3] = {CAN_TSR_TXOK0, CAN_TSR_TXOK1, CAN_TSR_TXOK2};
static const uint32_t ALST[3] = {CAN_TSR_ALST0, CAN_TSR_ALST1, CAN_TSR_ALST2};
static const uint32_t TERR[3] = {CAN_TSR_TERR0, CAN_TSR_TERR1, CAN_TSR_TERR2};
static const uint32_t ABRQ[3] = {CAN_TSR_ABRQ0, CAN_TSR_ABRQ1, CAN_TSR_ABRQ2};

void stub_write_reg(volatile uint32_t* reg, uint32_t val)
{
  int i;
  if (reg == &gCan.TSR)
  {
    for (i = 0; i < 3; i++)
    {
      if (val & RQCP[i]) gCan.TSR &= ~(RQCP[i] | TXOK[i] | ALST[i] | TERR[i]);
      if ((val & ABRQ[i]) && !(gCan.TSR & TME[i])) gCan.TSR |= TME[i] | RQCP[i];   /* aborted, no TXOK */
    }
  }
  else if (reg == &gCan.MCR)
  {
    uint32_t old = gCan.MCR;
    gCan.MCR = val;
    if (val & CAN_MCR_INRQ) gCan.MSR |= CAN_MSR_INAK; else gCan.MSR &= ~CAN_MSR_INAK;
    if ((old & CAN_MCR_INRQ) && !(val & CAN_MCR_INRQ))
    {
      gInitCycles++;
      if (gHealOnInit) { gBusOff = 0; gCan.ESR = 0; }
    }
  }
  else
  {
    *reg = val;
  }
}

uint32_t HAL_GetTick(void) { return gNow; }

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef* h, const CAN_TxHeaderTypeDef* hd, const uint8_t d[], uint32_t* mb)
{
  int i;
  (void)hd; (void)d;
  if (h->State != HAL_CAN_STATE_READY && h->State != HAL_CAN_STATE_LISTENING) return HAL_ERROR;   /* as the real HAL */
  if (gSwallow) { *mb = 1; return HAL_OK; }
  for (i = 0; i < 3; i++)
  {
    if (h->Instance->TSR & TME[i])
    {
      /* like the hardware: a new request clears the old completion flags of this mailbox */
      h->Instance->TSR &= ~(TME[i] | RQCP[i] | TXOK[i] | ALST[i] | TERR[i]);
      *mb = 1u << i;
      return HAL_OK;
    }
  }
  return HAL_ERROR;
}
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef* h, uint32_t it) { h->Instance->IER |= it; return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef* h)
{ CLEAR_BIT(h->Instance->MCR, CAN_MCR_INRQ); h->State = HAL_CAN_STATE_LISTENING; return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef* h, const CAN_FilterTypeDef* f) { (void)h; (void)f; return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef* h, uint32_t fifo, CAN_RxHeaderTypeDef* hd, uint8_t d[])
{ (void)h; (void)fifo; (void)hd; (void)d; return HAL_OK; }
void NVIC_SystemReset(void) { gResetCalls++; }

/* overrides the weak default in MCAN.c (as LOG.c does on target) */
void MCAN_ReportError(const uint8_t payload[6], uint8_t beforeReset)
{
  uint8_t code = payload[0];
  if (code < 8) { gRep[code]++; memcpy(gLast[code], payload, 6); }
  if (code == MCAN_ERR_TX_STALL)
  {
    if (payload[1] == eMTE_RECOVERY) gEvRecovery++;
    if (payload[1] == eMTE_RESET) gEvReset++;
  }
  if (beforeReset) gRepBeforeReset++;
}

static void BusStep(void)
{
  int i;
  if (gBusOff) return;
  for (i = 0; i < 3; i++)
  {
    if (!(gCan.TSR & TME[i])) { gCan.TSR |= TME[i] | RQCP[i] | TXOK[i]; gSentFrames++; }
  }
}

/* main loop: 1 ms bus, 5 ms MCAN_Transmit, 10 ms MCAN_Update_10ms; trafficPer10ms frames queued every 10 ms */
static void Run(uint32_t ms, int trafficPer10ms)
{
  uint32_t i; int k;
  uint8_t data[8] = {0};
  for (i = 0; i < ms; i++)
  {
    gNow++;
    BusStep();
    if (gNow % 5 == 0) MCAN_Transmit();
    if (gNow % 10 == 0)
    {
      for (k = 0; k < trafficPer10ms; k++) MCAN_SendFrame(0x187, data, 8);
      MCAN_Update_10ms();
    }
  }
}

static void Setup(void)
{
  memset(&gCan, 0, sizeof(gCan));
  memset(&gHcan, 0, sizeof(gHcan));
  gCan.TSR = TME[0] | TME[1] | TME[2];
  gCan.MCR = CAN_MCR_INRQ; gCan.MSR = CAN_MSR_INAK;   /* after HAL_CAN_Init(): init mode */
  gHcan.Instance = &gCan;
  gHcan.State = HAL_CAN_STATE_READY;
  gBusOff = 0; gHealOnInit = 0; gSwallow = 0; gInitCycles = 0; gResetCalls = 0; gSentFrames = 0;
  memset(gRep, 0, sizeof(gRep)); memset(gLast, 0, sizeof(gLast));
  gRepBeforeReset = 0; gEvRecovery = 0; gEvReset = 0;
  MCAN_Init(&gHcan, 7);
  MCAN_Start();
}

int main(void)
{
  const s_McanDiag* d;
  uint8_t data[8] = {0};
  int i, drops;

  /* 1. ABOM forced, error notifications, normal traffic: no stall, no report */
  Setup();
  CHECK(gCan.MCR & CAN_MCR_ABOM);
  CHECK(gHcan.Init.AutoBusOff == ENABLE);
  CHECK((gCan.IER & CAN_IT_BUSOFF) && (gCan.IER & CAN_IT_ERROR_PASSIVE));
  Run(70000, 1);
  d = MCAN_GetDiag();
  CHECK(d->stallRecoveryCnt == 0);
  CHECK(d->txOkCnt > 6900);
  CHECK(d->txOkCnt == gSentFrames);
  CHECK(d->txRingDropCnt == 0);
  CHECK(gIrqDisabled == 0);
  for (i = 0; i < 8; i++) CHECK(gRep[i] == 0);

  /* 2. idle node (heartbeat only, every 5 s) is neither a pending stall nor a dead TX */
  Setup();
  Run(60000, 0);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);
  CHECK(MCAN_GetDiag()->txOkCnt >= 11);

  /* 3. bus-off at t=1 s that heals on the init-mode cycle: one recovery after MCAN_TX_STALL_MS, then traffic again */
  Setup();
  Run(1000, 1);
  gBusOff = 1; gHealOnInit = 1;
  gCan.ESR = CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF | (255u << CAN_ESR_TEC_Pos) | (3u << CAN_ESR_REC_Pos);
  Run(MCAN_TX_STALL_MS - 50, 1);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);   /* not yet */
  CHECK(gRep[MCAN_ERR_ERRCNT] == 1);               /* but the error monitor reported the bus-off immediately */
  CHECK(gLast[MCAN_ERR_ERRCNT][1] == (0x80 | 0x07) && gLast[MCAN_ERR_ERRCNT][2] == 255);
  Run(200, 1);
  d = MCAN_GetDiag();
  CHECK(d->stallRecoveryCnt == 1);
  CHECK(gEvRecovery == 1 && gEvReset == 0);
  CHECK(gRep[MCAN_ERR_TX_STALL_SW] == 1);
  CHECK(gLast[MCAN_ERR_TX_STALL][2] == 255 && gLast[MCAN_ERR_TX_STALL][4] == 0x07);
  CHECK(gLast[MCAN_ERR_TX_STALL_SW][3] == 0x10);   /* all mailboxes pending (TME=0), ring not empty */
  CHECK(gLast[MCAN_ERR_TX_STALL_SW][5] == 1);      /* reason: pending */
  CHECK(gInitCycles == 2);                         /* HAL_CAN_Start + recovery */
  CHECK(d->esrAtStall & CAN_ESR_BOFF);
  CHECK(d->busOffCnt == 1 && d->tecMax == 255);
  CHECK(d->txAbortCnt == 3);
  CHECK(d->txHwBusyCnt > 0);
  CHECK(d->txRingMax == TX_BUFF_SIZE);
  CHECK(d->txRingDropCnt > 0);
  {
    uint32_t ok = d->txOkCnt;
    Run(5000, 1);
    CHECK(MCAN_GetDiag()->txOkCnt > ok + 400);
    CHECK(MCAN_GetDiag()->stallFailCnt == 0);
    CHECK(MCAN_GetDiag()->stallRecoveryCnt == 1);
  }

  /* 4. persistent stall (recovery does not help) */
  Setup();
  Run(1000, 1);
  gBusOff = 1; gHealOnInit = 0;
  Run(10 * MCAN_TX_STALL_MS + 100, 1);
  d = MCAN_GetDiag();
#if (MCAN_TX_STALL_RESET_AFTER > 0)
  CHECK(gResetCalls >= 1);
  CHECK(gEvReset >= 1);
  CHECK(gRepBeforeReset == gResetCalls);
  CHECK(gEvRecovery == MCAN_TX_STALL_RESET_AFTER);  /* first reset after RESET_AFTER failed recoveries */
#else
  CHECK(gResetCalls == 0 && gEvReset == 0);
  CHECK(d->stallRecoveryCnt == 10);
  CHECK(d->stallFailCnt == 10);
#endif

  /* 5. full ring: drops counted, MCAN_SendFrame returns 1 */
  Setup();
  drops = 0;
  for (i = 0; i < TX_BUFF_SIZE + 50; i++) drops += MCAN_SendFrame(0x187, data, 8);
  d = MCAN_GetDiag();
  CHECK(drops == 50);
  CHECK(d->txRingDropCnt == 50);
  CHECK(d->txRingMax == TX_BUFF_SIZE);
  CHECK(d->txRingLevel == TX_BUFF_SIZE);

  /* 6. tick wrap-around (49.7 days) does not trigger a false stall */
  Setup();
  gNow = 0xFFFFFFFFu - 2000u;
  MCAN_Start();
  Run(20000, 1);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);
  gNow = 0;

  /* 7. corrupted TX ring pointer: restored from the shadow within 10 ms, reported, TX continues */
  Setup();
  Run(1000, 1);
  {
    CAN_handle* h = MCAN_TestGetHandle();
    CB_handle good = h->TxBuf;
    uint32_t ok;
    h->TxBuf = NULL;
    Run(10, 1);
    CHECK(h->TxBuf == good);
    CHECK(MCAN_GetDiag()->ptrRepairCnt == 1);
    CHECK(gRep[MCAN_ERR_PTR_REPAIR] == 1 && gLast[MCAN_ERR_PTR_REPAIR][1] == 0x01);
    ok = MCAN_GetDiag()->txOkCnt;
    Run(2000, 1);
    CHECK(MCAN_GetDiag()->txOkCnt > ok + 150);
    CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);
  }

  /* 8. corrupted HAL state: AddTxMessage fails, ring fills, recovery puts the state back, TX continues */
  Setup();
  Run(1000, 1);
  gHcan.State = HAL_CAN_STATE_ERROR;
  Run(MCAN_TX_STALL_MS + 100, 1);
  d = MCAN_GetDiag();
  CHECK(d->stallRecoveryCnt == 1);
  CHECK(d->halStateRepairCnt == 1);
  CHECK(gLast[MCAN_ERR_TX_STALL_SW][1] == HAL_CAN_STATE_ERROR);
  CHECK(gLast[MCAN_ERR_TX_STALL_SW][3] == (0x07 | 0x10 | 0x20));   /* mailboxes free, ring full, state repaired */
  CHECK(gLast[MCAN_ERR_TX_STALL][2] == 0);                         /* TEC 0: not a bus problem */
  {
    uint32_t ok = d->txOkCnt;
    Run(2000, 1);
    CHECK(MCAN_GetDiag()->txOkCnt > ok + 150);
  }

  /* 9a. frames vanish under normal traffic: the ring is seen non-empty at the 10 ms samples -> pending rule (3 s) */
  Setup();
  Run(1000, 1);
  gSwallow = 1;
  Run(MCAN_TX_STALL_MS + 100, 1);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 1);
  CHECK(gLast[MCAN_ERR_TX_STALL_SW][5] == 1);
  gSwallow = 0;

  /* 9b. frames vanish, heartbeat only (nothing looks pending at the samples): caught by MCAN_TX_DEAD_MS */
  Setup();
  Run(1000, 0);
  gSwallow = 1;
  Run(MCAN_TX_DEAD_MS - 1200, 0);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);
  Run(400, 0);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 1);
  CHECK(gLast[MCAN_ERR_TX_STALL_SW][5] == 2);      /* reason: dead */
  gSwallow = 0;
  Run(6000, 0);                                    /* next heartbeat goes out */
  CHECK(MCAN_GetDiag()->stallFailCnt == 0);

  /* 10. error monitor: immediate report with holdoff, short bus-off seen only by HAL, window summary */
  Setup();
  Run(1000, 1);
  gCan.ESR = CAN_ESR_EWGF | (100u << CAN_ESR_TEC_Pos) | (5u << CAN_ESR_REC_Pos);
  gHcan.ErrorCode |= 0x01u | 0x08u;                /* HAL: EWG + stuff error */
  Run(20, 1);
  CHECK(gRep[MCAN_ERR_ERRCNT] == 1);
  CHECK(gLast[MCAN_ERR_ERRCNT][1] == 0x81 && gLast[MCAN_ERR_ERRCNT][2] == 100 && gLast[MCAN_ERR_ERRCNT][4] == 0x01);
  CHECK((gHcan.ErrorCode & 0x1FFu) == 0);          /* consumed */
  gCan.ESR = (40u << CAN_ESR_TEC_Pos);             /* falls below warning */
  Run(1000, 1);
  gHcan.ErrorCode |= 0x04u;                        /* short bus-off (ABOM recovered between samples) */
  Run(20, 1);
  CHECK(gRep[MCAN_ERR_ERRCNT] == 1);               /* holdoff */
  CHECK(MCAN_GetDiag()->busOffCnt == 1);
  CHECK(MCAN_GetDiag()->halErrSeen == (0x01 | 0x08 | 0x04));
  gCan.ESR = 0;
  Run(MCAN_ERRMON_WINDOW_MS, 1);
  CHECK(gRep[MCAN_ERR_ERRCNT] == 2);               /* one summary for the window */
  CHECK(gLast[MCAN_ERR_ERRCNT][1] == (0x01 | 0x04) && gLast[MCAN_ERR_ERRCNT][2] == 100);
  Run(MCAN_ERRMON_WINDOW_MS + 100, 1);
  CHECK(gRep[MCAN_ERR_ERRCNT] == 2);               /* quiet window: nothing */
  gCan.ESR = (20u << CAN_ESR_TEC_Pos);             /* low TEC without flags, above MCAN_ERRMON_LEVEL */
  Run(MCAN_ERRMON_WINDOW_MS + 100, 1);
  CHECK(gRep[MCAN_ERR_ERRCNT] == 3);
  CHECK(gLast[MCAN_ERR_ERRCNT][1] == 0 && gLast[MCAN_ERR_ERRCNT][2] == 20);

  if (fails)
  {
    printf("test_mcan: %d FAILED\n", fails);
    return 1;
  }
  printf("test_mcan OK (MCAN_TX_STALL_RESET_AFTER=%u)\n", (unsigned)MCAN_TX_STALL_RESET_AFTER);
  return 0;
}
