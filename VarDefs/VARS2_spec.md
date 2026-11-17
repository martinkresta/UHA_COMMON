# VARS2 — shared variable protocol, specification

Status: draft agreed 2026-10-31 (Martin + Claude). Single source of truth for
the variables: `uha_vars.yaml` (same folder). This document describes the
protocol, the YAML schema, the generated code and the migration plan.

---

## 1. Scope

- Replaces only the **variable sharing** protocol (today `CMD_VAR_VALUE`
  `0x110 + nodeId`, int16 values, and its forwarding ELECON → RPi via
  RPISERP `0x221`).
- **Not changed:** buttons, gestures, remote requests, `CMD_BALANCE_INFO`,
  RTC sync, SysInfo, log, UHAMON, NMT/heartbeat, bootloader.
- Application/control logic is not changed until migration step 6
  (except rescaling where needed).

---

## 2. CAN frame

| Item | Value |
|------|-------|
| COB-ID | `0x180 + nodeId` (sender). Old format `0x110 + nodeId` is no longer sent since step 6b |
| DLC | 8 |

```
byte  0..1   id      uint16, little-endian
byte  2..5   value   4 B, little-endian; float32 | int32 | uint32 per definition
byte  6      flags   bit0 VALID   – implemented
                     bit1 FORCED  – reserved (value forced from PC tool)
                     bit2..7      – reserved, send 0
byte  7      seq     reserved (sequence counter 0..255), send 0
```

- The value type is **not** transmitted; both sides know it from the
  definition (by ID).
- Receivers ignore unknown IDs and reserved bits/bytes.
- Compatibility rules: an existing variable never changes its **ID** or
  **type**; IDs are never reused.

---

## 3. Definition file `uha_vars.yaml`

```yaml
version: <int>
nodes:    {NAME: nodeId, ...}            # RPI = 15 (receive only)
defaults: {type: float, timeout_ms: auto, on_can: true}
enums:    {EnumName: {0: Name, 1: Name, ...}}
groups:
  <Group>:                                # CamelCase, = member of Vars2
    producer: <NODE>                      # one producer per group (on the group or its container)
    struct: <Type>                        # optional: shared C type sVars<Type> (identical fields)
    validity: group | per_field           # default group: VARS2_SetGroupValid(); per_field: by id
    send: periodic | on_change            # default periodic; inherited container -> subgroup -> var
    vars:                                 # either vars ...
      - id: <uint16>
        name: <UPPER_SNAKE>               # VID_<name>, name in uha2.json
        field: <CamelCase_unit>           # C field, unit suffix _pct _V _A _W _Wh _Ah _C _l _lpm _Pa _ppm _mps
        type: float|int32|uint32|bool|enum   (default float)
        enum: <EnumName>                  # if type: enum
        unit: "<unit>"
        period_ms: <int>                  # publish period of the producer (VARS2 TX); on_change: refresh
        send: periodic | on_change        # on_change: TX on change of value/valid (min 1 s apart) + period
        timeout_ms: <int>|auto            # auto = 3 x period_ms (receiver: .valid cleared)
        on_can: false                     # not on CAN, only through the ELECON tunnel to the RPi
        count: <n>                        # array: ids id .. id+n-1
        receivers: [NODE, ...]            # documentation only
        legacy: VAR_...                   # optional old int16 name, kept for UhaGatePy (uha.json); not used by the FW
        legacy_scale: <number>            # old int16 raw = physical * legacy_scale (uha.json only)
        log_s: 10 | 60                    # UhaGatePy: DB log period (agg avg / delta)
        agg: avg | delta | event          # UhaGatePy: event = one row per change (table events), no log_s
        power_w: <number>                 # documentation only: load of a light / output in W
        desc: "<text>"
  <Container>:                            # ... or subgroups (max 3 levels: Vars2.Container.Group.Field)
    subgroups:
      <Group>: {producer, validity, send, vars}
```

**States** (`groups.States`, step 2a, `UHA_COMMON/tasks/vars2_states_on_change.md`): discrete states (lights,
relays, modes, setpoints) as normal variables, one subgroup per producing node (`States.IoboardU`, ...),
`send: on_change`, `period_ms: 20000` (refresh; `timeout_ms: auto` = 60000 still fits the u16 `TimeoutMs`),
`agg: event`, no `legacy`. Id ranges: IOBOARD_U 400–419, IOBOARD_D 420–439, TECHM 440–479, ELECON 480–489,
ELECON_D 490–509, RECU 510–519. Lights: `uint32`, `%` 0–100 (output really set).

Validator rules (`tools/vars_validate.py`):
- unique `id` (arrays expanded), `name` and `field`; `producer`, `receivers` reference existing nodes;
  `enum` exists; `count` ranges do not overlap other IDs;
- one producer per group (no `producer` on a variable), field names with the unit suffix, groups sharing a
  `struct` have identical fields, at most one level of `subgroups`;
- **ELM rule:** `ELM.c` writes power to `energy id + 10` — for every `CONS_*_WH` in `Meters` the variable at
  `id + 10` must exist, be `POW_*_W` and be in the same group;
- compatibility check against the previous released YAML: no ID/type change, no removed ID reused;
- `send` is periodic | on_change; `agg: event` must not have `log_s`, `agg: avg | delta` need `log_s`.

---

## 4. Generated FW code (UHA_COMMON)

Every variable is a typed `{value, valid}` item (8 B): `sVarF32` (float), `sVarI32` (int32, bool, enum),
`sVarU32` (uint32), see `Inc/VARS2_types.h`. Values are in physical units. IDs as `VID_<NAME>`.

### `uha_vars_gen.h`

```c
typedef enum { VID_BAT_SOC = 10, VID_BAT_VOLTAGE_V = 11, /* ... */ } eVarId;
typedef enum { ElHeaterState_Heating = 0, /* ... */ } eElHeaterState;

typedef struct { sVarF32 Soc_pct; sVarF32 Voltage_V; /* ... */ } sVarsGlobalEnergyBattery;
typedef struct { sVarsGlobalEnergyBattery Battery; sVarsGlobalEnergySolar Solar;
                 sVarsGlobalEnergyLoad Load; } sVarsGlobalEnergy;           /* container */
typedef struct { sVarF32 Soc_pct; /* ... */ sVarF32 CellVoltage_V[16]; } sVarsPack;  /* shared struct */

typedef struct { sVarsSystem System; sVarsGlobalEnergy GlobalEnergy; sVarsPack PackHome80Ah; /* ... */ } sVars;
extern sVars Vars2;   /* Vars2.GlobalEnergy.Battery.Soc_pct.value / .valid */
```

### `uha_vars_gen.c`

```c
sVars Vars2;
const sVarMeta VarMeta[VARS_COUNT] = {     /* sorted by id, 16 B each; flags VMF_ON_CAN | VMF_ON_CHANGE */
/* pVar                            id  period tmout  type      producer       flags */
  { &Vars2.GlobalEnergy.Battery.Soc_pct, 10, 1000, 3000, VT_FLOAT, NODEID_ELECON, VMF_ON_CAN },
};
const uint16_t VarIdToIndex[VAR_MAX_ID + 1];   /* VAR_IDX_UNKNOWN = unknown id */
/* + compile-time checks: every struct is a plain array of 8-byte items */
```

### Hand-written module `VARS2.c/.h` (UHA_COMMON)

```c
void    VARS2_Init(uint8_t thisNodeId);               /* from COM_Init(): all 0 / invalid */
void    VARS2_Update_10ms(void);                      /* from COM_Update_10ms(): TX + receive timeout */
uint8_t VARS2_ProcessRxFrame(uint16_t cobId, uint8_t dlc, uint8_t rtr, const uint8_t* data);   /* from COM */
void    VARS2_UpdateLocal_10ms(VARS2_FrameHook cb);   /* ELECON: on_can: false variables to the tunnel */
void    VARS2_SetFrameHook(VARS2_FrameHook hook);     /* ELECON: every RX / TX frame to the tunnel */

/* producer API */
VARS2_SetGroupValid(pGroup, valid);                   /* .valid of every field of a group / container */
VarSet(&pGroup->Field, value, valid);                 /* one field (VarSetI32 / VarSetU32 for int types) */
uint8_t VARS2_SetById(eVarId id, float value, uint8_t valid);   /* by id (TEMP, ELM, SENSIRION) */
```

- **Ownership:** `Vars2` is used by `APP.c` (wiring) and `VARS2.c` only. Each module gets pointers to its
  groups in its `Init()`: non-const for the groups it produces, `const` for the groups it reads.
  Example: `GEMON_Init(&Vars2.GlobalEnergy, &Vars2.HomeEnergy, &Vars2.WorkshopEnergy)`.
- **TX:** VARS2 sends every variable this node produces by `period_ms` (CAN 0x180+node, max 2 frames per
  10 ms = `VARS2_CAN_MAX_PER_CALL`, round robin); `on_can: false` variables only through `VARS2_UpdateLocal_10ms()` (ELECON tunnel).
  The frame carries `.value` and `.valid` as they are at send time.
- **send: on_change** (`VMF_ON_CHANGE`): an own variable is also due when its 4-byte value or valid differ
  from what was **last sent**, at least `VARS2_ON_CHANGE_MIN_MS` (1000) after its last send (`OnChangeDue()`
  in `VARS2.c`); `period_ms` stays as refresh. Same for the CAN path and the tunnel path. The modules just
  write `Vars2`; a change that returns to the sent value within 1 s is not sent. Frame, RX and timeout unchanged.
- **RX:** value and valid from the frame into `Vars2` (not for own or unknown ids); `.valid` is cleared
  when no frame arrived for `timeout_ms`. Readers only look at `.valid`.
- **State:** `mState` (private in `VARS2.c`, 8 B per row) holds `RxTick` / `TxTick`; for own on_change rows
  the last sent value shares the bytes of the unused `RxTick` (union) and the last sent valid the padding.
- Everything runs in the main loop (no `Vars2` access from interrupts).

---

## 5. Other consumers of the YAML

- **RPi gateway UhaGatePy (Python):** reads `uha_vars.yaml` directly at runtime (no codegen).
- **PC tool UhaCanMaster (C#):** reads `uha_vars.yaml` directly at runtime (no codegen).

---

## 6. Migration plan

| Step | What | Notes |
|------|------|-------|
| 0 | YAML, validator, FW generator | Python, in `UHA_COMMON/VarDefs` |
| 1 | All nodes: send old + new format, receive both; control uses old data | ~2× traffic (~140 frames/s, ~7 %) |
| 2 | ELECON → RPi bridge: VARS2 tunnel over RPISERP (§7), old `0x221` kept | ELECON FW only |
| 3 | New RPi gateway **UhaGatePy** (Python) reads VARS2 tunnel + old `0x221` | UhaGate6 (C++) stays as backup |
| 4 | Flash ELECON + RPi, verify data | |
| 5 | PC tool on new format | old format not supported in PC tool |
| 6a | Nodes switch reading to new format, one by one | any order (all send both) |
| 6b | Old format removed: `0x110` TX in `COM.c` (part A), `0x110` RX per node and ELECON `0x221` (part B) | after all read new; task `UHA_COMMON/tasks/vars2_stop_legacy_tx.md` |
| 7 | Direct access: modules work with `Vars2` (`{value, valid}`, physical units), old `Vars[]` / `VAR_*` API, `COM_AddStreamedVariable()` and the legacy conversion removed | task `UHA_COMMON/tasks/vars2_direct_access.md`, notes `direct-access-notes.md` (2026-11-14) |

Dependency map: `vars_dependency_map.md` (only mutual dependency:
ELECON ↔ ELECON_D).

---

## 7. ELECON → RPi link: VARS2 tunnel over RPISERP

The RPi is not on CAN. ELECON (node 4) forwards data to the RPi over a
USB-serial link (UART, 115200 Bd, 8N1) using the existing **RPISERP**
framing. Step 2 adds the VARS2 tunnel; everything existing stays.

### 7.1 RPISERP framing (unchanged)

```
byte 0     0x7F
byte 1     0xAA
byte 2     len        payload length = 2 + data length (2..10)
byte 3..4  packet id  uint16, BIG-endian (hi, lo)
byte 5..   data       len - 2 bytes (0..8)
last       checksum   8-bit sum of all preceding bytes of the frame (from 0x7F)
```
Max frame 14 bytes. Payload max 10 bytes (`RSP_PACKET_SIZE` 14 in ELECON).

### 7.2 Packet ids

| Packet id | Dir | Data | Status |
|-----------|-----|------|--------|
| `0x221` | ELECON → RPi | old variable: `id_hi id_lo val_hi val_lo valid_hi valid_lo` (int16, big-endian), from ELECON's scan list | **kept** until migration step 6b |
| `0x620` | ELECON → RPi | node log message, 8 bytes (byte 0 = node id) | kept |
| `0x051` | RPi → ELECON | RTC sync (4 B big-endian time) | kept |
| **`0x052`** | RPi → ELECON | **command** `CMD_RPI_COMMAND`: target, cmd id u16 LE, value 4 B LE, seq (`CMD_spec.md` §2); ELECON forwards it to CAN `0x200 + target` | **new** (step 3) |
| `0x050`, `0x120`, `0x713`, `0x777` | RPi → ELECON | var value / set var / read var request / master HB | kept, unused by the gateway today |
| **`0x180 + n`** (`0x180..0x18F`) | ELECON → RPi | **VARS2 tunnel:** the 8 data bytes of a VARS2 CAN frame, unchanged (spec §2: `id u16 LE, value 4B LE, flags, seq`). `n` = producer node id | **new** |

Note: inside the tunnel payload the variable id and value are
**little-endian** (copied from CAN), while the RPISERP packet id is
big-endian. Payload = 2 + 8 = 10 bytes (fits).

### 7.3 What ELECON puts into the tunnel

1. **Every VARS2 frame received from CAN** (any node), immediately, packet id =
   the CAN COB-ID (`0x180 + producer`).
2. **Every VARS2 frame ELECON itself sends on CAN** (its `on_can` variables),
   packet id `0x184`.
3. **ELECON-local variables with `on_can: false`** (producer ELECON, flag
   `VMF_ON_CAN` not set in `VarMeta`): sent **only** over the tunnel, packet id
   `0x184`, with `VarMeta.PeriodMs`, value / validity from `Vars2`.

The RPi does not distinguish 1/2/3 — producer = `packet id & 0x0F`.

### 7.4 Throughput

- Old scan list: up to 100 packets/s (unchanged, 1 per 10 ms).
- Tunnel: ~70/s (VARS2 on CAN) + ~44/s (ELECON-local) ≈ 115/s.
- Total ≈ 215 packets/s; link capacity at 115200 Bd ≈ 800 frames/s (14 B).
- **Today ELECON transmits max 1 RPISERP frame per 10 ms (`RSP_TransmitFromFifo`
  from `SCOM_Update_10ms`) with a TX FIFO of 10 frames** — step 2 must raise
  this (start the next DMA transfer on TX-complete, larger FIFO) so that
  ≥ 300 frames/s are possible without drops; FIFO overflows must be counted.

### 7.5 Gateway side (UhaGatePy)

- Accepts `0x221` (old) and `0x180..0x18F` (VARS2) in parallel.
- Per variable: prefer the VARS2 value while it is fresh (age ≤ `timeout_ms`),
  otherwise the old value.
- Old-format outputs (`uha.json`, `bms.json`) are produced from the preferred
  value converted back with `legacy_scale` (`round(value * legacy_scale)`), so
  the web and other readers are unchanged.

## 8. Commands (`commands:` section)

The YAML also holds the command channel (requests RPi → ELECON → CAN `0x200 + target`), specified in
**`CMD_spec.md`** (format, ELECON forwarding, node side, YAML rules). The `commands:` section does not change
the VARS2 generated files; `tools/gen_all.py` validates it and writes `uha_cmds_gen.h/.c` (`eCmdId`,
`CmdMeta[]`, `CMD_<NAME>_MIN/_MAX`). Feedback of a command is the VARS2 `state` variable it names. RPISERP
packet id `0x52` = `CMD_RPI_COMMAND` (RPi → ELECON).
