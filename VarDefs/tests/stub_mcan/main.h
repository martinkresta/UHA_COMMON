/*
 * main.h - PC stub for test_mcan.c ONLY (not used on target).
 * Simulated bxCAN register block with the hardware semantics MCAN.c relies on:
 *  - TSR RQCPx are rc_w1 (writing 1 clears RQCPx/TXOKx/ALSTx/TERRx), ABRQx aborts a pending mailbox
 *  - MCR INRQ sets / clears MSR INAK immediately
 * Register writes go through WRITE_REG/SET_BIT/CLEAR_BIT, which are mapped to stub functions here
 * (on target they are the plain CMSIS macros).
 */
#ifndef STUB_MCAN_MAIN_H_
#define STUB_MCAN_MAIN_H_

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
#define ENABLE  1u
#define DISABLE 0u

typedef struct { uint32_t dummy; } GPIO_TypeDef;

typedef struct
{
  volatile uint32_t MCR, MSR, TSR, RF0R, RF1R, IER, ESR, BTR;
} CAN_TypeDef;

typedef struct { uint32_t AutoBusOff; } CAN_InitTypeDef;
typedef enum
{
  HAL_CAN_STATE_RESET = 0, HAL_CAN_STATE_READY, HAL_CAN_STATE_LISTENING, HAL_CAN_STATE_SLEEP_PENDING,
  HAL_CAN_STATE_SLEEP_ACTIVE, HAL_CAN_STATE_ERROR
} HAL_CAN_StateTypeDef;
typedef struct { CAN_TypeDef* Instance; CAN_InitTypeDef Init; volatile HAL_CAN_StateTypeDef State; volatile uint32_t ErrorCode; } CAN_HandleTypeDef;

typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; uint32_t TransmitGlobalTime; } CAN_TxHeaderTypeDef;
typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_RxHeaderTypeDef;
typedef struct
{
  uint32_t FilterIdHigh, FilterIdLow, FilterMaskIdHigh, FilterMaskIdLow, FilterFIFOAssignment, FilterBank,
           FilterMode, FilterScale, FilterActivation, SlaveStartFilterBank;
} CAN_FilterTypeDef;

#define CAN_RTR_DATA 0u
#define CAN_ID_STD   0u
#define CAN_FILTER_FIFO0 0u
#define CAN_FILTER_FIFO1 1u
#define CAN_RX_FIFO0 0u
#define CAN_RX_FIFO1 1u
#define CAN_FILTERMODE_IDMASK 0u
#define CAN_FILTERMODE_IDLIST 1u
#define CAN_FILTERSCALE_16BIT 0u

#define CAN_IT_RX_FIFO0_MSG_PENDING 0x002u
#define CAN_IT_RX_FIFO1_MSG_PENDING 0x010u
#define CAN_IT_ERROR_WARNING        0x100u
#define CAN_IT_ERROR_PASSIVE        0x200u
#define CAN_IT_BUSOFF               0x400u
#define CAN_IT_LAST_ERROR_CODE      0x800u
#define CAN_IT_ERROR                0x8000u

#define CAN_MCR_INRQ 0x001u
#define CAN_MCR_ABOM 0x040u
#define CAN_MSR_INAK 0x001u

#define CAN_TSR_RQCP0 (1u << 0)
#define CAN_TSR_TXOK0 (1u << 1)
#define CAN_TSR_ALST0 (1u << 2)
#define CAN_TSR_TERR0 (1u << 3)
#define CAN_TSR_ABRQ0 (1u << 7)
#define CAN_TSR_RQCP1 (1u << 8)
#define CAN_TSR_TXOK1 (1u << 9)
#define CAN_TSR_ALST1 (1u << 10)
#define CAN_TSR_TERR1 (1u << 11)
#define CAN_TSR_ABRQ1 (1u << 15)
#define CAN_TSR_RQCP2 (1u << 16)
#define CAN_TSR_TXOK2 (1u << 17)
#define CAN_TSR_ALST2 (1u << 18)
#define CAN_TSR_TERR2 (1u << 19)
#define CAN_TSR_ABRQ2 (1u << 23)
#define CAN_TSR_TME0  (1u << 26)
#define CAN_TSR_TME1  (1u << 27)
#define CAN_TSR_TME2  (1u << 28)

#define CAN_ESR_EWGF 0x01u
#define CAN_ESR_EPVF 0x02u
#define CAN_ESR_BOFF 0x04u
#define CAN_ESR_TEC_Pos 16u
#define CAN_ESR_TEC  (0xFFu << CAN_ESR_TEC_Pos)
#define CAN_ESR_REC_Pos 24u
#define CAN_ESR_REC  (0xFFu << CAN_ESR_REC_Pos)

void     stub_write_reg(volatile uint32_t* reg, uint32_t val);
#define READ_REG(REG)         (REG)
#define WRITE_REG(REG, VAL)   stub_write_reg(&(REG), (VAL))
#define SET_BIT(REG, BIT)     stub_write_reg(&(REG), (REG) | (BIT))
#define CLEAR_BIT(REG, BIT)   stub_write_reg(&(REG), (REG) & ~(BIT))

uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef* h, const CAN_TxHeaderTypeDef* hd, const uint8_t d[], uint32_t* mb);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef* h, uint32_t it);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef* h);
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef* h, const CAN_FilterTypeDef* f);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef* h, uint32_t fifo, CAN_RxHeaderTypeDef* hd, uint8_t d[]);
void NVIC_SystemReset(void);

extern int gIrqDisabled;
static inline uint32_t __get_PRIMASK(void) { return (uint32_t)gIrqDisabled; }
static inline void __set_PRIMASK(uint32_t v) { gIrqDisabled = (int)v; }
static inline void __disable_irq(void) { gIrqDisabled = 1; }
static inline void __enable_irq(void) { gIrqDisabled = 0; }

#endif
