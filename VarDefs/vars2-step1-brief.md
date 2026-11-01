# Task: VARS2 step 1 — VARS2 module, dual transmission (old + new format) on all nodes

## 1. Context

Read first: `UHA_FW/CLAUDE.md`, `UHA_COMMON/CLAUDE.md`,
`UHA_COMMON/VarDefs/VARS2_spec.md` (**the specification**),
`UHA_COMMON/VarDefs/README.md`, `UHA_COMMON/VarDefs/vars_dependency_map.md`.

Step 0 is done (branch `vars2-step0` in `UHA_COMMON`): `uha_vars.yaml`,
validator, generator, `generated/uha_vars_gen.h/.c`, `VARS2_types.h`.

**Step 1 goal (spec §4 "Migration-phase behaviour", §6 step 1):**
every node keeps doing exactly what it does today on the old protocol, and
additionally

- **sends every variable it streams today also in the new VARS2 frame**
  (`0x180 + nodeId`, `[id u16 LE | value 4B LE | flags | seq]`), value
  converted from the old int16 (`raw / LegacyScale`), validity taken from
  the old `Vars[]`;
- **receives VARS2 frames** and stores them in `Vars2` + per-value state
  (valid flag, receive tick).

**Nothing in the application reads `Vars2` yet.** Control logic, old
`VARS.c`, `VAR_GetVariable()`, the old frame `0x110 + nodeId`, all other
messages and the ELECON → RPi bridge (`ASW_ELECON/SCOM.c`) stay unchanged.

## 2. Design (follow it; ask if something does not fit the code)

### 2.1 Files

- Generated `uha_vars_gen.h` → `UHA_COMMON/Inc/`, `uha_vars_gen.c` →
  `UHA_COMMON/Src/`, `VARS2_types.h` → `UHA_COMMON/Inc/`.
  Change the generator's default output accordingly (keep `-o`), update
  `tests/compile_check.py` and `VarDefs/README.md`. `VarDefs/generated/`
  is removed.
- New hand-written `UHA_COMMON/Inc/VARS2.h`, `UHA_COMMON/Src/VARS2.c`.
- `UHA_COMMON/Src` and `Inc` are linked into **every** ASW project — check
  in each project's `.project`/`.cproject` that the new files are really
  compiled (resource filters / exclusions) and that include paths work.

### 2.2 VARS2 API (spec §4)

```c
void    VARS2_Init(uint8_t thisNodeId);
void    VARS2_ProcessFrame(const uint8_t* data);   /* RX of 0x180+n */
void    VARS2_SendLegacy(uint16_t legacyVarId);     /* TX, see 2.3 */
void    VARS2_SetValid(eVarId id, uint8_t valid);   /* producer side, default valid */
uint8_t VARS2_IsValid(eVarId id);                   /* valid flag && not timed out */
```

- Per-value RAM state `{ uint8_t Valid; uint32_t RxTick; }` indexed like
  `VarMeta`. Timeout from `VarMeta.TimeoutMs`, time base `HAL_GetTick()`.
- Unknown IDs and reserved bits/bytes are ignored. Values are copied as
  4 raw bytes into `*VarMeta[idx].pValue` (little-endian, Cortex-M is LE).

### 2.3 Hook points — in `UHA_COMMON/COM.c` only (no ASW change expected)

- **TX:** in `COM.c` `SendVariable(id)` (used by the stream list and by
  `COM_SendVariable`), after the old frame is queued call
  `VARS2_SendLegacy(id)`:
  - `idx = VarIdToIndex[id]` (via the legacy id; in this step legacy id ==
    VARS2 id); unknown → nothing;
  - `raw = Vars[id].value`, `valid = Vars[id].valid`;
  - `VT_FLOAT`: `float v = (float)raw / LegacyScale`;
    `VT_INT32/UINT32/BOOL/ENUM`: integer value of `raw`;
  - write the value also into `Vars2` (so `Vars2` is complete on the
    producer node too), build the frame (flags bit0 = valid, `seq` = 0) and
    queue it with `MCAN_SendFrame(0x180 + nodeId, data, 8)`.
  - So the new format is sent for **exactly the same set and periods** as
    today. The generated per-node publish lists (`VarMeta.Producer`,
    `PeriodMs`) are **not** used for TX in this step.
- **RX:** in `COM_GetRxMessage()`: frames with `(StdId & 0x7F0) == 0x180`
  are passed to `VARS2_ProcessFrame()` and **not** returned to the
  application (the loop continues with the next frame). All nodes read CAN
  through `COM_GetRxMessage()` (verified in every `APP.c`). The CAN filter
  FIFO1 (0x000–0x6FF) already passes 0x18x.
- `VARS2_Init()` from `COM_Init()`.
- Do not change `MCAN.c`, `VARS.c`, `VARS.h`, scheduler, ASW code.
  If a change outside `COM.c` / new files turns out necessary, ask Martin.

### 2.4 Load and timing (check, report)

- `COM_Update_10ms()` sends at most one streamed variable per 10 ms tick →
  now 2 frames per tick; `MCAN_Transmit()` (5 ms tick) sends one frame per
  call → ~200 frames/s per node, TX buffer 100. Verify no TX buffer overflow
  for the busiest node (ELECON_D, ~32 vars/s today → ~64 frames/s).
- Total bus load expected ~70 → ~140 frames/s (~7 %).

## 3. Constraints

- Branch `vars2-step1` in `UHA_COMMON` (from `vars2-step0`; `git status`
  first, uncommitted changes in `vars2-step0` — e.g. the YAML scale fixes —
  commit them to `vars2-step0` first). Same branch name in any other repo
  you have to touch (should be none).
- **No version bumps, no `Deployment/` artifacts, no flashing of live
  nodes** — Martin releases and flashes.
- Build **all six nodes**: ASW_ELECON, ASW_ELECON_D, ASW_TECHM, ASW_IOBOARD_D,
  ASW_IOBOARD_U, ASW_RECU_NUCLEO (EVSE is not on the bus — skip). Use the
  STM32CubeIDE headless build or the generated makefiles with the CubeIDE
  toolchain; if you cannot build, find out why and ask Martin. No new
  warnings. Report flash/RAM before → after for each node (from the `.map`
  or `arm-none-eabi-size`).
- Desk test only (ELECON_TEST board on the desk bus, Waveshare/PCAN, see
  `C:\Martin\GIT\C#\UhaCanMaster\docs\`): OTA of the ELECON_TEST build is
  allowed **only on the desk bus**. Live network: listen only.

## 4. Work plan — subagents

1. Study (spec, COM.c, MCAN.c, VARS.c, generated files, all six projects'
   build config). Write the plan to `UHA_COMMON/VarDefs/step1-notes.md`.
   No stop needed unless something contradicts this brief.
2. You implement `VARS2.c/.h` and the generator output move.
3. Parallel subagents:
   - (a) **COM.c hooks** + review of all six projects' build configuration
     (files compiled, include paths);
   - (b) **PC-side unit test** of `VARS2.c` with gcc (stub HAL): RX of
     sample frames, unknown id, timeout, `SendLegacy` conversion for float /
     int / bool / enum / `LegacyScale` 0.1, 10, 100, 1000; frame byte layout
     vs spec §2. Example: old `BAT_VOLTAGE_V10` raw 532 → new frame value
     53.2f; `BMS1_CELL8_MV` (id 187) raw 3326 → 3.326f.
4. Build all six nodes; fix warnings.
5. **Independent reviewer subagent** (did not write the code): checks the
   result against `VARS2_spec.md` and this brief — no behaviour change of
   the old protocol, frame layout, endianness, no new blocking/RAM risks,
   build config of every project.
6. Desk test with the ELECON_TEST board: old and new frames both present,
   decoded values equal (UhaCanMaster terminal / variable watch decodes
   VARS and VARS2), bus load, no TX overflow. Record results in
   `step1-notes.md`.
7. Small logical commits on `vars2-step1`. Do not merge.

## 5. Report to Martin

What changed (files), build results + flash/RAM per node, unit-test and
desk-test results (incl. VARS vs VARS2 value comparison), bus load, open
points, and the recommended order to flash the live nodes.
