/* AUTO-GENERATED from uha_vars.yaml - DO NOT EDIT
 * YAML version: 0
 * regenerate with tools/gen_all.py
 * command channel (CMD_spec.md): used by CMD.c (CMD_ProcessFrame) and APP_OnCommand()
 */

#ifndef UHA_CMDS_GEN_H_
#define UHA_CMDS_GEN_H_

#include <stdint.h>
#include "VARS2_types.h"   /* eVarType VT_* */

#define CMDS_COUNT   31
#define CMD_MAX_ID   706

/* ---- command ids ---- */
typedef enum
{
  CMDID_NONE                  =   0,  /* never sent */
  CMDID_LIGHT_WASHBASIN       = 101,  /* IOBOARD_D, uint32 0..100 %, state ST_LIGHT_WASHBASIN */
  CMDID_LIGHT_KITCHEN_COUNTER = 102,  /* IOBOARD_D, uint32 0..100 %, state ST_LIGHT_KITCHEN_COUNTER */
  CMDID_LIGHT_LOFT_WEST       = 201,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LOFT_WEST */
  CMDID_LIGHT_LOFT_EAST_1     = 202,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LOFT_EAST_1 */
  CMDID_LIGHT_LOFT_EAST_2     = 203,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LOFT_EAST_2 */
  CMDID_LIGHT_BEDROOM_SOUTH   = 204,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_BEDROOM_SOUTH */
  CMDID_LIGHT_BEDROOM_NORTH   = 205,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_BEDROOM_NORTH */
  CMDID_LIGHT_BATHROOM        = 206,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_BATHROOM */
  CMDID_LIGHT_STAIR_STEPS     = 207,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_STAIR_STEPS */
  CMDID_LIGHT_STAIRS          = 208,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_STAIRS */
  CMDID_LIGHT_HALLWAY         = 209,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_HALLWAY */
  CMDID_LIGHT_LIVING_ROOM     = 210,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_LIVING_ROOM */
  CMDID_LIGHT_DINING_ROOM     = 211,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_DINING_ROOM */
  CMDID_LIGHT_KITCHEN         = 212,  /* IOBOARD_U, uint32 0..100 %, state ST_LIGHT_KITCHEN */
  CMDID_SCREEN_EAST           = 213,  /* IOBOARD_U, enum eBlindCmd 0..3, state ST_SCREEN_EAST */
  CMDID_SCREEN_WEST           = 214,  /* IOBOARD_U, enum eBlindCmd 0..3, state ST_SCREEN_WEST */
  CMDID_UP_REQ_DAY            = 301,  /* TECHM, float 5..25 degC, state ST_UP_REQ_DAY_C */
  CMDID_UP_REQ_NIGHT          = 302,  /* TECHM, float 5..25 degC, state ST_UP_REQ_NIGHT_C */
  CMDID_DOWN_REQ_DAY          = 303,  /* TECHM, float 5..25 degC, state ST_DOWN_REQ_DAY_C */
  CMDID_DOWN_REQ_NIGHT        = 304,  /* TECHM, float 5..25 degC, state ST_DOWN_REQ_NIGHT_C */
  CMDID_EXHAUST_SET           = 305,  /* TECHM, float 150..250 degC, state ST_EXHAUST_SET_C */
  CMDID_AC_3KW                = 401,  /* ELECON, bool 0..1, state ST_AC_3KW */
  CMDID_AC_5KW                = 402,  /* ELECON, bool 0..1, state ST_AC_5KW */
  CMDID_RECU_CO2_SETPOINT     = 501,  /* RECU, uint32 400..1200 ppm, state ST_RECU_CO2_SETPOINT */
  CMDID_RECU_ANTIDRY_RH       = 502,  /* RECU, float 20..70 %, state ST_RECU_ANTIDRY_RH */
  CMDID_LIGHT_KOLARNA         = 701,  /* ELECON_D, uint32 0..100 %, state ST_LIGHT_KOLARNA */
  CMDID_LIGHT_SKLEP           = 702,  /* ELECON_D, uint32 0..100 %, state ST_LIGHT_SKLEP */
  CMDID_AKUPACK_SET_DAY       = 703,  /* ELECON_D, float 0..15 degC, state ST_AKUPACK_SET_DAY_C */
  CMDID_AKUPACK_SET_NIGHT     = 704,  /* ELECON_D, float 0..15 degC, state ST_AKUPACK_SET_NIGHT_C */
  CMDID_AC_AXPERT             = 705,  /* ELECON_D, bool 0..1, state ST_AC_AXPERT */
  CMDID_AC_VICTRON            = 706,  /* ELECON_D, bool 0..1, state ST_AC_VICTRON */
} eCmdId;

/* ---- limits: the target module clamps to these (one source of truth for UI and FW) ---- */
#define CMD_LIGHT_WASHBASIN_MIN       0u
#define CMD_LIGHT_WASHBASIN_MAX       100u
#define CMD_LIGHT_KITCHEN_COUNTER_MIN 0u
#define CMD_LIGHT_KITCHEN_COUNTER_MAX 100u
#define CMD_LIGHT_LOFT_WEST_MIN       0u
#define CMD_LIGHT_LOFT_WEST_MAX       100u
#define CMD_LIGHT_LOFT_EAST_1_MIN     0u
#define CMD_LIGHT_LOFT_EAST_1_MAX     100u
#define CMD_LIGHT_LOFT_EAST_2_MIN     0u
#define CMD_LIGHT_LOFT_EAST_2_MAX     100u
#define CMD_LIGHT_BEDROOM_SOUTH_MIN   0u
#define CMD_LIGHT_BEDROOM_SOUTH_MAX   100u
#define CMD_LIGHT_BEDROOM_NORTH_MIN   0u
#define CMD_LIGHT_BEDROOM_NORTH_MAX   100u
#define CMD_LIGHT_BATHROOM_MIN        0u
#define CMD_LIGHT_BATHROOM_MAX        100u
#define CMD_LIGHT_STAIR_STEPS_MIN     0u
#define CMD_LIGHT_STAIR_STEPS_MAX     100u
#define CMD_LIGHT_STAIRS_MIN          0u
#define CMD_LIGHT_STAIRS_MAX          100u
#define CMD_LIGHT_HALLWAY_MIN         0u
#define CMD_LIGHT_HALLWAY_MAX         100u
#define CMD_LIGHT_LIVING_ROOM_MIN     0u
#define CMD_LIGHT_LIVING_ROOM_MAX     100u
#define CMD_LIGHT_DINING_ROOM_MIN     0u
#define CMD_LIGHT_DINING_ROOM_MAX     100u
#define CMD_LIGHT_KITCHEN_MIN         0u
#define CMD_LIGHT_KITCHEN_MAX         100u
#define CMD_SCREEN_EAST_MIN           0
#define CMD_SCREEN_EAST_MAX           3
#define CMD_SCREEN_WEST_MIN           0
#define CMD_SCREEN_WEST_MAX           3
#define CMD_UP_REQ_DAY_MIN            5.0f
#define CMD_UP_REQ_DAY_MAX            25.0f
#define CMD_UP_REQ_NIGHT_MIN          5.0f
#define CMD_UP_REQ_NIGHT_MAX          25.0f
#define CMD_DOWN_REQ_DAY_MIN          5.0f
#define CMD_DOWN_REQ_DAY_MAX          25.0f
#define CMD_DOWN_REQ_NIGHT_MIN        5.0f
#define CMD_DOWN_REQ_NIGHT_MAX        25.0f
#define CMD_EXHAUST_SET_MIN           150.0f
#define CMD_EXHAUST_SET_MAX           250.0f
#define CMD_AC_3KW_MIN                0
#define CMD_AC_3KW_MAX                1
#define CMD_AC_5KW_MIN                0
#define CMD_AC_5KW_MAX                1
#define CMD_RECU_CO2_SETPOINT_MIN     400u
#define CMD_RECU_CO2_SETPOINT_MAX     1200u
#define CMD_RECU_ANTIDRY_RH_MIN       20.0f
#define CMD_RECU_ANTIDRY_RH_MAX       70.0f
#define CMD_LIGHT_KOLARNA_MIN         0u
#define CMD_LIGHT_KOLARNA_MAX         100u
#define CMD_LIGHT_SKLEP_MIN           0u
#define CMD_LIGHT_SKLEP_MAX           100u
#define CMD_AKUPACK_SET_DAY_MIN       0.0f
#define CMD_AKUPACK_SET_DAY_MAX       15.0f
#define CMD_AKUPACK_SET_NIGHT_MIN     0.0f
#define CMD_AKUPACK_SET_NIGHT_MAX     15.0f
#define CMD_AC_AXPERT_MIN             0
#define CMD_AC_AXPERT_MAX             1
#define CMD_AC_VICTRON_MIN            0
#define CMD_AC_VICTRON_MAX            1

typedef struct
{
  uint16_t Id;       /* eCmdId */
  uint8_t  Target;   /* node id (NODEID_*) */
  uint8_t  Type;     /* eVarType: how the 4 value bytes are read */
} sCmdMeta;

extern const sCmdMeta CmdMeta[];   /* CMDS_COUNT entries, sorted by id */

#endif /* UHA_CMDS_GEN_H_ */
