/*
 * main.h - PC stub for compile_check.py ONLY (not used on target).
 * Minimal stand-in for the STM32 CubeMX main.h so that the real
 * UHA_COMMON/Inc/VARS.h and MCAN.h compile with host gcc.
 */

#ifndef STUB_MAIN_H_
#define STUB_MAIN_H_

#include <stdint.h>

typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_RxHeaderTypeDef;
typedef struct { uint32_t StdId; uint32_t ExtId; uint32_t IDE; uint32_t RTR; uint32_t DLC; } CAN_TxHeaderTypeDef;
typedef struct { void* Instance; } CAN_HandleTypeDef;

#endif /* STUB_MAIN_H_ */
