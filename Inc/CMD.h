/*
 * CMD.h - command channel, node side (UHA_COMMON/VarDefs/CMD_spec.md section 4)
 *
 *  Commands are requests to the node that owns the function; the node decides, local safety wins.
 *  CAN frame id 0x200 + target node (CMD_NODE_COMMAND in COM.h), DLC 8:
 *      [cmd_lo, cmd_hi, v0, v1, v2, v3, seq, 0]   cmd id u16 LE, value 4 bytes LE by the command's type
 *  COM_GetRxMessage() consumes these frames and passes the own ones to CMD_ProcessFrame().
 *  Feedback = the VARS2 state of the function (no ACK). Ids, types and limits: uha_vars.yaml `commands:`
 *  -> uha_cmds_gen.h (eCmdId, CMD_<NAME>_MIN/_MAX).
 */

#ifndef INC_CMD_H_
#define INC_CMD_H_

#include <stdint.h>
#include "uha_cmds_gen.h"


/* Looks the command id up in CmdMeta[], ignores unknown ids and ids whose target is not this node
 * (COM_GetNodeId()), converts the 4 value bytes by type (float / int32 / uint32; bool and enum as int32;
 * a non-finite float is ignored) and calls APP_OnCommand(). No range checks here: the target clamps.
 * data: the 8 data bytes of the CAN frame. Returns 1 if APP_OnCommand() was called, 0 otherwise. */
uint8_t CMD_ProcessFrame(const uint8_t* data);

/* Implemented per node (APP.c): one switch over eCmdId that calls the existing module APIs and clamps
 * the value to CMD_<NAME>_MIN/_MAX. A weak empty default in CMD.c keeps nodes without commands building. */
void APP_OnCommand(eCmdId id, float value);

/* Clamp helper for the APP_OnCommand() handlers: value limited to min..max, NaN -> min. */
float CMD_Clamp(float value, float min, float max);


#endif /* INC_CMD_H_ */
