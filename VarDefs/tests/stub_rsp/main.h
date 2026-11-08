/*
 * main.h - PC stub for test_tunnel.c ONLY (not used on target).
 * Fake UART/DMA registers so that ASW_ELECON RPISERP.c and SCOM.c can be
 * compiled and driven on the host.
 */
#ifndef STUB_RSP_MAIN_H_
#define STUB_RSP_MAIN_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_RxHeaderTypeDef;
typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_TxHeaderTypeDef;
typedef struct { void* Instance; } CAN_HandleTypeDef;
#define CAN_RTR_DATA 0u

uint32_t HAL_GetTick(void);

typedef struct { volatile uint32_t CR1, CR2, CR3, ISR, ICR, RDR, TDR; } USART_TypeDef;
typedef struct { volatile uint32_t CCR, CNDTR, CPAR, CMAR; } DMA_Channel_TypeDef;
typedef struct { volatile uint32_t IFCR; } DMA_TypeDef;
typedef struct { uint32_t BaudRate, WordLength, StopBits, Parity, Mode, HwFlowCtl, OverSampling, OneBitSampling; } UART_InitTypeDef;
typedef struct { uint32_t AdvFeatureInit; } UART_AdvFeatureInitTypeDef;
typedef struct { USART_TypeDef* Instance; UART_InitTypeDef Init; UART_AdvFeatureInitTypeDef AdvancedInit; } UART_HandleTypeDef;
typedef struct { DMA_Channel_TypeDef* Instance; } DMA_HandleTypeDef;

extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_TypeDef gDma1;
#define DMA1 (&gDma1)

#define DMA_CCR_EN     0x01u
#define DMA_CCR_TCIE   0x02u
#define DMA_CCR_DIR    0x10u
#define DMA_CCR_MINC   0x80u
#define DMA_IFCR_CGIF4 0x1000u
#define USART_CR1_UE     0x01u
#define USART_CR1_RE     0x04u
#define USART_CR1_TE     0x08u
#define USART_CR1_IDLEIE 0x10u
#define USART_CR1_TCIE   0x40u
#define USART_CR3_DMAR   0x40u
#define USART_CR3_DMAT   0x80u
#define USART_CR3_OVRDIS 0x1000u
#define USART_ISR_IDLE   0x10u
#define USART_ISR_TC     0x40u
#define USART_ICR_IDLECF 0x10u
#define USART_ICR_TCCF   0x40u
#define UART_WORDLENGTH_8B 0u
#define UART_STOPBITS_1 0u
#define UART_PARITY_NONE 0u
#define UART_MODE_TX_RX 0u
#define UART_HWCONTROL_NONE 0u
#define UART_OVERSAMPLING_16 0u
#define UART_ONE_BIT_SAMPLE_DISABLE 0u
#define UART_ADVFEATURE_NO_INIT 0u
#define HAL_OK 0
typedef struct { void* Instance; } RTC_HandleTypeDef;
typedef struct { uint32_t dummy; } GPIO_TypeDef;

/* interrupt masking - the test counts how deep the critical section is */
extern int gIrqDisabled;
static inline uint32_t __get_PRIMASK(void) { return (uint32_t)gIrqDisabled; }
static inline void __set_PRIMASK(uint32_t v) { gIrqDisabled = (int)v; }
static inline void __disable_irq(void) { gIrqDisabled = 1; }
static inline void __enable_irq(void) { gIrqDisabled = 0; }

static inline int HAL_UART_Init(UART_HandleTypeDef* h) { (void)h; return HAL_OK; }
static inline void Error_Handler(void) {}

#endif
