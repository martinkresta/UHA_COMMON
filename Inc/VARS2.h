/*
 * VARS2.h
 *
 * VARS2 shared variable protocol - migration step 1 (dual old+new TX, RX into
 * Vars2). See UHA_COMMON/VarDefs/VARS2_spec.md.
 *
 * Hand-written, NOT generated. Uses the generated tables in uha_vars_gen.h.
 */

#ifndef INC_VARS2_H_
#define INC_VARS2_H_

#include "main.h"
#include "uha_vars_gen.h"

/* call once from COM_Init() */
void VARS2_Init(uint8_t thisNodeId);

/* RX: pass the 8-byte payload of a 0x180+node frame */
void VARS2_ProcessFrame(const uint8_t* data);

/* TX: value + validity taken from the old Vars[] via legacyVarId (== VARS2
 * id in this migration step); builds and queues the 0x180+node frame.
 * No-op if legacyVarId has no VARS2 definition yet. */
void VARS2_SendLegacy(uint16_t legacyVarId);

/* producer side: set validity of a locally produced value (default after
 * VARS2_Init() is valid) */
void VARS2_SetValid(eVarId id, uint8_t valid);

/* valid flag && not timed out (VarMeta.TimeoutMs since last update) */
uint8_t VARS2_IsValid(eVarId id);

#endif /* INC_VARS2_H_ */
