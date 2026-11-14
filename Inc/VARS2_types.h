/*
 * VARS2_types.h - types for the generated VARS2 tables (uha_vars_gen.h/.c)
 *
 * Hand-written, NOT generated. Layout of sVarMeta must match the row order
 * emitted by tools/vars_gen_fw.py.
 */

#ifndef VARS2_TYPES_H_
#define VARS2_TYPES_H_

#include <stdint.h>
#include <stddef.h>

/* value type of a variable (4-byte value, not transmitted on CAN) */
typedef enum
{
  VT_FLOAT  = 0,
  VT_INT32  = 1,
  VT_UINT32 = 2,
  VT_BOOL   = 3,   /* stored as int32, 0 / 1 */
  VT_ENUM   = 4    /* stored as int32 */
} eVarType;

/* One variable in Vars2: value in physical units + validity.
 * All three have the same layout (4-byte value at offset 0, valid at offset 4,
 * 8 bytes), so every group struct is a plain array of 8-byte items. */
typedef struct { float    value; uint8_t valid; } sVarF32;
typedef struct { int32_t  value; uint8_t valid; } sVarI32;   /* int32, enum, bool */
typedef struct { uint32_t value; uint8_t valid; } sVarU32;

#define VAR_ITEM_SIZE    8u
#define VAR_VALID_OFFSET 4u

typedef char VARS2_SIZE_sVarF32[(sizeof(sVarF32) == VAR_ITEM_SIZE && offsetof(sVarF32, valid) == VAR_VALID_OFFSET) ? 1 : -1];
typedef char VARS2_SIZE_sVarI32[(sizeof(sVarI32) == VAR_ITEM_SIZE && offsetof(sVarI32, valid) == VAR_VALID_OFFSET) ? 1 : -1];
typedef char VARS2_SIZE_sVarU32[(sizeof(sVarU32) == VAR_ITEM_SIZE && offsetof(sVarU32, valid) == VAR_VALID_OFFSET) ? 1 : -1];

#define VMF_ON_CAN       0x01u   /* published on CAN */

#define VAR_IDX_UNKNOWN  0xFFFFu /* VarIdToIndex: unknown id */

typedef struct
{
  void*    pVar;          /* -> sVarF32 / sVarI32 / sVarU32 field in Vars2 */
  uint16_t Id;
  uint16_t PeriodMs;
  uint16_t TimeoutMs;
  uint8_t  Type;          /* eVarType */
  uint8_t  Producer;      /* NODEID_* */
  uint8_t  Flags;         /* VMF_* */
} sVarMeta;               /* 16 B on ARM */

#endif /* VARS2_TYPES_H_ */
