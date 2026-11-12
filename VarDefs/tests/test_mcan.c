/*
 * test_mcan.c - PC unit test of the MCAN TX supervision (feature/can_tx_supervision).
 * Built twice by test_mcan.py: with the default MCAN_TX_STALL_RESET_AFTER (0 = never reset) and with 2.
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
static int gInitCycles;      /* INRQ 1 -> 0 transitions */
static int gResetCalls;
static uint32_t gSentFrames;
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
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef* h) { CLEAR_BIT(h->Instance->MCR, CAN_MCR_INRQ); return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef* h, const CAN_FilterTypeDef* f) { (void)h; (void)f; return HAL_OK; }
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef* h, uint32_t fifo, CAN_RxHeaderTypeDef* hd, uint8_t d[])
{ (void)h; (void)fifo; (void)hd; (void)d; return HAL_OK; }
void NVIC_SystemReset(void) { gResetCalls++; }

/* overrides the weak default in MCAN.c (as LOG.c does on target) */
void MCAN_TxStallCallback(e_McanTxEvent event, const s_McanDiag* diag)
{
  (void)diag;
  if (event == eMTE_RECOVERY) gEvRecovery++;
  if (event == eMTE_RESET) gEvReset++;
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
  gBusOff = 0; gHealOnInit = 0; gInitCycles = 0; gResetCalls = 0; gSentFrames = 0; gEvRecovery = 0; gEvReset = 0;
  MCAN_Init(&gHcan, 7);
  MCAN_Start();
}

int main(void)
{
  const s_McanDiag* d;
  uint8_t data[8] = {0};
  int i, drops;

  /* 1. ABOM forced, error notifications, normal traffic: no stall */
  Setup();
  CHECK(gCan.MCR & CAN_MCR_ABOM);
  CHECK(gHcan.Init.AutoBusOff == ENABLE);
  CHECK((gCan.IER & CAN_IT_BUSOFF) && (gCan.IER & CAN_IT_ERROR_PASSIVE));
  Run(20000, 1);
  d = MCAN_GetDiag();
  CHECK(d->stallRecoveryCnt == 0);
  CHECK(d->txOkCnt > 1900);              /* 2000 data frames + heartbeats, all counted */
  CHECK(d->txOkCnt == gSentFrames);
  CHECK(d->txRingDropCnt == 0);
  CHECK(gIrqDisabled == 0);

  /* 2. idle node (heartbeat only) is not a stall */
  Setup();
  Run(30000, 0);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);
  CHECK(MCAN_GetDiag()->txOkCnt >= 5);   /* heartbeat every 5 s */

  /* 3. bus-off at t=1 s that heals on the init-mode cycle: one recovery after MCAN_TX_STALL_MS, then traffic again */
  Setup();
  Run(1000, 1);
  gBusOff = 1; gHealOnInit = 1;
  gCan.ESR = CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF | (255u << CAN_ESR_TEC_Pos) | (3u << CAN_ESR_REC_Pos);
  Run(MCAN_TX_STALL_MS - 50, 1);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);   /* not yet */
  Run(200, 1);
  d = MCAN_GetDiag();
  CHECK(d->stallRecoveryCnt == 1);
  CHECK(gEvRecovery == 1 && gEvReset == 0);
  CHECK(gInitCycles == 2);                         /* HAL_CAN_Start + recovery */
  CHECK(d->esrAtStall & CAN_ESR_BOFF);
  CHECK(d->busOffCnt == 1 && d->tecMax == 255);
  CHECK(d->txAbortCnt == 3);                       /* 3 stuck mailboxes aborted */
  CHECK(d->txHwBusyCnt > 0);
  CHECK(d->txRingMax == TX_BUFF_SIZE);             /* ring filled during the stall ... */
  CHECK(d->txRingDropCnt > 0);                     /* ... and the drops were counted */
  {
    uint32_t ok = d->txOkCnt;
    Run(5000, 1);
    CHECK(MCAN_GetDiag()->txOkCnt > ok + 400);     /* transmitting again */
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
  CHECK(gEvRecovery == MCAN_TX_STALL_RESET_AFTER);  /* first reset request after RESET_AFTER failed recoveries */
#else
  CHECK(gResetCalls == 0 && gEvReset == 0);
  CHECK(d->stallRecoveryCnt == 10);                 /* one attempt per MCAN_TX_STALL_MS */
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

  /* 6. TICK wrap-around (49.7 days) does not trigger a false stall */
  Setup();
  gNow = 0xFFFFFFFFu - 2000u;
  MCAN_Start();
  Run(10000, 1);
  CHECK(MCAN_GetDiag()->stallRecoveryCnt == 0);

  if (fails)
  {
    printf("test_mcan: %d FAILED\n", fails);
    return 1;
  }
  printf("test_mcan OK (MCAN_TX_STALL_RESET_AFTER=%u)\n", (unsigned)MCAN_TX_STALL_RESET_AFTER);
  return 0;
}
