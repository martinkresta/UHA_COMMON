/* AUTO-GENERATED from uha_vars.yaml - DO NOT EDIT
 * YAML version: 0
 * regenerate with tools/gen_all.py
 * command channel (CMD_spec.md): used by CMD.c (CMD_ProcessFrame) and APP_OnCommand()
 */

#ifndef UHA_CMDS_GEN_H_
#define UHA_CMDS_GEN_H_

#include <stdint.h>
#include "VARS2_types.h"   /* eVarType VT_* */

#define CMDS_COUNT   12
#define CMD_MAX_ID   212

/* ---- command ids ---- */
typedef enum
{
  CMDID_NONE                =   0,  /* never sent */
  CMDID_LIGHT_LOFT_WEST     = 201,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LOFT_WEST */
  CMDID_LIGHT_LOFT_EAST_1   = 202,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LOFT_EAST_1 */
  CMDID_LIGHT_LOFT_EAST_2   = 203,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LOFT_EAST_2 */
  CMDID_LIGHT_BEDROOM_SOUTH = 204,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_BEDROOM_SOUTH */
  CMDID_LIGHT_BEDROOM_NORTH = 205,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_BEDROOM_NORTH */
  CMDID_LIGHT_BATHROOM      = 206,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_BATHROOM */
  CMDID_LIGHT_STAIR_STEPS   = 207,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_STAIR_STEPS */
  CMDID_LIGHT_STAIRS        = 208,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_STAIRS */
  CMDID_LIGHT_HALLWAY       = 209,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_HALLWAY */
  CMDID_LIGHT_LIVING_ROOM   = 210,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LIVING_ROOM */
  CMDID_LIGHT_DINING_ROOM   = 211,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_DINING_ROOM */
  CMDID_LIGHT_KITCHEN       = 212,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_KITCHEN */
} eCmdId;

/* ---- limits: the target module clamps to these (one source of truth for UI and FW) ---- */
#define CMD_LIGHT_LOFT_WEST_MIN     0u
#define CMD_LIGHT_LOFT_WEST_MAX     100u
#define CMD_LIGHT_LOFT_EAST_1_MIN   0u
#define CMD_LIGHT_LOFT_EAST_1_MAX   100u
#define CMD_LIGHT_LOFT_EAST_2_MIN   0u
#define CMD_LIGHT_LOFT_EAST_2_MAX   100u
#define CMD_LIGHT_BEDROOM_SOUTH_MIN 0u
#define CMD_LIGHT_BEDROOM_SOUTH_MAX 100u
#define CMD_LIGHT_BEDROOM_NORTH_MIN 0u
#define CMD_LIGHT_BEDROOM_NORTH_MAX 100u
#define CMD_LIGHT_BATHROOM_MIN      0u
#define CMD_LIGHT_BATHROOM_MAX      100u
#define CMD_LIGHT_STAIR_STEPS_MIN   0u
#define CMD_LIGHT_STAIR_STEPS_MAX   100u
#define CMD_LIGHT_STAIRS_MIN        0u
#define CMD_LIGHT_STAIRS_MAX        100u
#define CMD_LIGHT_HALLWAY_MIN       0u
#define CMD_LIGHT_HALLWAY_MAX       100u
#define CMD_LIGHT_LIVING_ROOM_MIN   0u
#define CMD_LIGHT_LIVING_ROOM_MAX   100u
#define CMD_LIGHT_DINING_ROOM_MIN   0u
#define CMD_LIGHT_DINING_ROOM_MAX   100u
#define CMD_LIGHT_KITCHEN_MIN       0u
#define CMD_LIGHT_KITCHEN_MAX       100u

typedef struct
{
  uint16_t Id;       /* eCmdId */
  uint8_t  Target;   /* node id (NODEID_*) */
  uint8_t  Type;     /* eVarType: how the 4 value bytes are read */
} sCmdMeta;

extern const sCmdMeta CmdMeta[];   /* CMDS_COUNT entries, sorted by id */

#endif /* UHA_CMDS_GEN_H_ */
