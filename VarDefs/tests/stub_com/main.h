/*
 * main.h - PC stub for test_com.c ONLY (not used on target).
 * Minimal stand-in for the STM32 CubeMX main.h so that the real
 * UHA_COMMON COM.c, VARS.c and VARS2.c compile with host gcc.
 */

#ifndef STUB_COM_MAIN_H_
#define STUB_COM_MAIN_H_

#include <stdint.h>
#include <stddef.h>

typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_RxHeaderTypeDef;
typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_TxHeaderTypeDef;
typedef struct { void* Instance; } CAN_HandleTypeDef;
#define CAN_RTR_DATA 0u
typedef struct { void* Instance; } RTC_HandleTypeDef;   /* RTC.h (included by COM.c) */

uint32_t HAL_GetTick(void);   /* mocked in test_com.c */

#endif /* STUB_COM_MAIN_H_ */
