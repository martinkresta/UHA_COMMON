/*
 * VARS2.h
 *
 * VARS2 shared variable protocol: the variables live in the generated Vars2
 * (value in physical units + valid flag per field). Producers write their
 * groups directly, VARS2 sends this node's variables by their YAML period
 * (CAN 0x180+node, or the tunnel callback for on_can: false) and receives
 * the other nodes' variables into Vars2. See UHA_COMMON/VarDefs/VARS2_spec.md.
 *
 * Vars2 is meant for APP.c (wiring: passes group pointers to the modules)
 * and VARS2.c only. Modules get pointers to their groups in their Init().
 *
 * Hand-written, NOT generated. Uses the generated tables in uha_vars_gen.h.
 */

#ifndef INC_VARS2_H_
#define INC_VARS2_H_

#include "main.h"
#include "uha_vars_gen.h"
#include <stddef.h>

/* call once from COM_Init(): all variables 0 and invalid */
void VARS2_Init(uint8_t thisNodeId);

/* call every 10 ms (from COM_Update_10ms()):
 * - TX: sends this node's on_can variables that are due by VarMeta.PeriodMs
 *   on CAN (0x180+node, max 1 frame per call) and passes each frame to the
 *   frame hook;
 * - RX: clears .valid of received variables older than VarMeta.TimeoutMs. */
void VARS2_Update_10ms(void);

/* Call every 10 ms on the node that tunnels to the RPi (ELECON): for this
 * node's variables NOT on CAN that are due by VarMeta.PeriodMs, builds the
 * frame from Vars2 and passes it to cb (nothing is sent on CAN). Max 4
 * frames per call. */
typedef void (*VARS2_FrameHook)(uint16_t cobId, const uint8_t* data8);
void VARS2_UpdateLocal_10ms(VARS2_FrameHook cb);

/* Optional hook called with every VARS2 frame received (RX path) and every
 * frame this node sends on CAN: COB-ID + the 8 data bytes. Default: none.
 * Used by ELECON to tunnel VARS2 to the RPi (spec sec.7). */
void VARS2_SetFrameHook(VARS2_FrameHook hook);

/* RX entry used by COM: accepts only DLC == 8 and non-RTR frames (returns 0
 * and drops anything else), otherwise stores value + valid into Vars2 (not
 * for this node's own variables or unknown ids) and calls the hook; returns 1. */
uint8_t VARS2_ProcessRxFrame(uint16_t cobId, uint8_t dlc, uint8_t rtr, const uint8_t* data);


/* ---- producer API ---- */

/* sets .valid of every field of a group or a container (all subgroups):
 * VARS2_SetGroupValid(pStorage, valid); */
void VARS2_SetGroupValidRaw(void* pGroup, size_t size, uint8_t valid);
#define VARS2_SetGroupValid(pGroup, valid)  VARS2_SetGroupValidRaw((pGroup), sizeof(*(pGroup)), (valid))

/* single field: value + validity */
static inline void VarSet(sVarF32* pVar, float value, uint8_t valid)
{
	pVar->value = value;
	pVar->valid = valid;
}

/* By id (modules configured with VID_* ids: TEMP, ELM, SENSIRION): writes
 * value (physical units; rounded for integer types) and validity of one of
 * this node's variables. Returns 0 and writes nothing for an unknown id or a
 * variable produced by another node. */
uint8_t VARS2_SetById(eVarId id, float value, uint8_t valid);


/* ---- temporary compatibility for unconverted node code (VARS.c wrappers),
 * removed in part C of the direct-access task ---- */

/* VAR_SetVariable(): own variable from the old int16 raw value (float: raw /
 * LegacyScale, integer types: raw). Returns 0 and writes nothing for an
 * unknown id or a variable produced by another node. */
uint8_t VARS2_SetLegacy(uint16_t legacyVarId, int16_t raw, uint8_t valid);

/* VAR_GetVariable(): value converted to the old int16 (float: value *
 * LegacyScale rounded to nearest, clamped). Sets INVALID_FLAG-style bit
 * invalidFlag in *invalid (may be NULL) if the value is invalid; returns 0
 * with the flag set for an unknown id. */
int16_t VARS2_GetLegacy(uint16_t legacyVarId, uint16_t* invalid, uint16_t invalidFlag);

/* COM_SendVariable(): sends one variable now (CAN 0x180+node + hook) */
void VARS2_SendLegacy(uint16_t legacyVarId);

#endif /* INC_VARS2_H_ */
