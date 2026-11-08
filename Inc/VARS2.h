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

/* Optional hook called with every VARS2 frame received (RX path) and sent
 * (TX path of VARS2_SendLegacy()): COB-ID + the 8 data bytes. Default: none.
 * Used by ELECON to tunnel VARS2 to the RPi (spec sec.7). */
typedef void (*VARS2_FrameHook)(uint16_t cobId, const uint8_t* data8);
void VARS2_SetFrameHook(VARS2_FrameHook hook);

/* RX entry used by COM: accepts only DLC == 8 and non-RTR frames (returns 0
 * and drops anything else), otherwise ProcessFrame() + hook; returns 1. */
uint8_t VARS2_ProcessRxFrame(uint16_t cobId, uint8_t dlc, uint8_t rtr, const uint8_t* data);

/* Call every 10 ms: for this node's variables NOT on CAN (producer == this
 * node, VMF_ON_CAN not set) that are due by VarMeta.PeriodMs, builds the
 * frame like VARS2_SendLegacy() (value + validity from the old Vars[]) and
 * passes it to cb (nothing is sent on CAN). Max 4 frames per call. */
void VARS2_UpdateLocal_10ms(VARS2_FrameHook cb);

/* TX: value + validity taken from the old Vars[] via legacyVarId (== VARS2
 * id in this migration step); builds and queues the 0x180+node frame.
 * No-op if legacyVarId has no VARS2 definition yet. */
void VARS2_SendLegacy(uint16_t legacyVarId);

/* producer side: set validity of a locally produced value (default after
 * VARS2_Init() is valid) */
void VARS2_SetValid(eVarId id, uint8_t valid);

/* Read layer (VAR_GetVariable() on nodes with VAR_EnableVars2Read()): the
 * received VARS2 value converted back to the legacy int16 (float: value *
 * LegacyScale rounded to nearest). Returns 0 = caller uses the old Vars[]:
 * unknown id, own variable (Producer == this node) or nothing received yet.
 * If the value is invalid or timed out, INVALID_FLAG is set in *invalid
 * (may be NULL) and the last value is still returned. */
uint8_t VARS2_GetLegacy(uint16_t legacyVarId, int16_t* value, uint16_t* invalid);

/* valid flag && not timed out (VarMeta.TimeoutMs since last update) */
uint8_t VARS2_IsValid(eVarId id);

#endif /* INC_VARS2_H_ */
