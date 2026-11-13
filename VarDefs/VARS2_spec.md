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
  <group>:                                # one C struct per group
    producer: <NODE>                      # default for the group
    vars:
      - id: <uint16>
        name: <UPPER_SNAKE>               # physical units, unit suffix in name
        legacy: VAR_...                   # old define (migration)
        type: float|int32|uint32|bool|enum   (default float)
        enum: <EnumName>                  # if type: enum
        unit: "<unit>"
        legacy_scale: <number>            # old int16 raw = physical * legacy_scale (default 1)
        producer: <NODE>                  # overrides group producer
        period_ms: <int>                  # publish period
        timeout_ms: <int>|auto            # auto = 3 x period_ms
        on_can: false                     # not on CAN (today ELECON-local, RPi only)
        count: <n>                        # array: ids id .. id+n-1
        receivers: [NODE, ...]            # documentation only
        desc: "<text>"
```

Validator rules:
- unique `id` (arrays expanded) and unique `name`;
- `producer`, `receivers` reference existing nodes; `enum` exists;
- `count` ranges do not overlap other IDs;
- **ELM rule:** `ELM.c` writes power to `energy id + 10` — for every ELM
  energy variable (`CONS_*_WH` written via `ELM_AddMeter`) the variable at
  `id + 10` must exist and be `POW_*_W`;
- compatibility check against the previous released YAML: no ID/type change,
  no removed ID reused.

---

## 4. Generated FW code (UHA_COMMON)

Naming: field names in LEMON style — CamelCase from the YAML name, unit
suffix after underscore, group prefix dropped (`BAT_VOLTAGE_V` →
`BatVoltage_V`, `BMS1_SOC` in group bms1 → `Soc`). IDs as `VID_<NAME>`.

### `uha_vars_gen.h`

```c
typedef enum { VID_BAT_SOC = 10, VID_BAT_VOLTAGE_V = 11, /* ... */ } eVarId;

typedef enum { ElHeaterState_Heating = 0, /* ... */ } eElHeaterState;

typedef struct { float BatSoc; float BatVoltage_V; /* ... */ } sVarsGlobalEnergy;
typedef struct { float Soc; float CellVoltage_V[16]; /* ... */ } sVarsBms1;
/* ... one struct per group, only 4-byte field types ... */

typedef struct { sVarsGlobalEnergy GlobalEnergy; sVarsBms1 Bms1; /* ... */ } sVars;
extern sVars Vars2;

#define VARS_COUNT  <number of values, arrays expanded>
#define VAR_MAX_ID  <highest id>
```

### `uha_vars_gen.c`

```c
sVars Vars2;

const sVarMeta VarMeta[VARS_COUNT] = {
/* id  type      pValue                          producer         period timeout legacy_scale legacyId */
  {10, VT_FLOAT, &Vars2.GlobalEnergy.BatSoc,     NODEID_ELECON,   1000,  3000,   1.0f,        VAR_BAT_SOC},
  /* ... */
};
const uint16_t VarIdToIndex[VAR_MAX_ID + 1];   /* 0xFFFF = unknown id */
```

Only variables with `on_can: true` are published on CAN.

### Hand-written module `VARS2.c/.h` (UHA_COMMON)

```c
void    VARS2_Init(uint8_t thisNodeId);
void    VARS2_ProcessFrame(const uint8_t* data);   /* RX 0x180+node, called by COM */
void    VARS2_Update_10ms(void);                    /* TX of this node's variables */
void    VARS2_SetValid(eVarId id, uint8_t valid);   /* producer side */
uint8_t VARS2_IsValid(eVarId id);                   /* RX flag && not timed out */
```

- Per-value state in RAM: `Valid`, `RxTick` (~8 B per value).
- **Validity:** producer side only via `VARS2_SetValid()`; default after
  start = **valid**; modules invalidate when needed.
- Receive side: VALID flag from the frame + timeout (`timeout_ms`).
- TX pacing: like today, one frame per 10 ms tick per node.

### Migration-phase behaviour (steps 1–5)

- TX takes the value **and validity** from the old `Vars[]`
  (`legacyId`), converts `int16 / legacy_scale` → physical value, sends
  the new frame. (Until step 6b the old frame `0x110` was sent as well; removed in 6b.)
- RX of new frames writes into `Vars2` (+ state); nothing in the
  application reads it yet.
- Old `VARS.c` / `VAR_GetVariable()` and the old protocol stay unchanged.

### Later (step 6 and beyond)

- Application modules read `Vars2.<Group>.<Field>` + `VARS2_IsValid()`.
- Modules with "modern" structures (LEMON, GEMON) get a pointer to their
  generated group instance and write directly (no copy), e.g.
  `LEMON_Init(..., &Vars2.Storage2)`; same type reused for several groups
  (storage1/storage2).

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
   `0x184`, with `VarMeta.PeriodMs`, value/validity converted from the old
   `Vars[]` exactly like `VARS2_SendLegacy` (spec §4).

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
