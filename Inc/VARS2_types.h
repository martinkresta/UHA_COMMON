/*
 * VARS2_types.h - types for the generated VARS2 tables (uha_vars_gen.h/.c)
 *
 * Hand-written, NOT generated. Layout of sVarMeta must match the row order
 * emitted by tools/vars_gen_fw.py.
 */

#ifndef VARS2_TYPES_H_
#define VARS2_TYPES_H_

#include <stdint.h>

/* value type of a variable (4-byte storage, not transmitted on CAN) */
typedef enum
{
  VT_FLOAT  = 0,
  VT_INT32  = 1,
  VT_UINT32 = 2,
  VT_BOOL   = 3,   /* stored as uint32_t, 0 / 1 */
  VT_ENUM   = 4    /* stored as uint32_t */
} eVarType;

#define VMF_ON_CAN       0x01u   /* published on CAN */

#define VAR_IDX_UNKNOWN  0xFFFFu /* VarIdToIndex: unknown id */
#define VAR_LEGACY_NONE  0xFFFFu /* no legacy id */

typedef struct
{
  void*    pValue;        /* -> field in Vars2 (4-byte value) */
  float    LegacyScale;   /* old int16 raw = physical * LegacyScale */
  uint16_t Id;
  uint16_t PeriodMs;
  uint16_t TimeoutMs;
  uint16_t LegacyId;      /* old VARS.h id, VAR_LEGACY_NONE if none */
  uint8_t  Type;          /* eVarType */
  uint8_t  Producer;      /* NODEID_* */
  uint8_t  Flags;         /* VMF_* */
} sVarMeta;               /* 20 B on ARM */

#endif /* VARS2_TYPES_H_ */
