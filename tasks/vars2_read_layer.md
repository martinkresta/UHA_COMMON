# Task: VARS2 read layer — nodes read remote variables from Vars2

Migration step 6a. The legacy API `VAR_GetVariable()` stays, modules do not
change. On a node where the layer is enabled, it returns remote variables from
`Vars2`, converted back to the legacy int16 value.

**How this task is run:** Part A once, then Part B for one node at a time.
Martin says which part/node to do. Do only that.

## Context

Read first: `UHA_FW/CLAUDE.md`, `UHA_COMMON/CLAUDE.md`, the target project's
`CLAUDE.md`, `UHA_COMMON/VarDefs/VARS2_spec.md`,
`UHA_COMMON/VarDefs/vars_dependency_map.md`, `UHA_COMMON/Src/VARS.c`,
`UHA_COMMON/Src/VARS2.c`.

**Prerequisite:** `feature/elecon_vars2_tunel` is merged to `main` in
`UHA_COMMON` and `ASW_ELECON`.

Facts checked in the code:
- All nodes still send old + VARS2 format, so nodes can switch in any order.
- `Vars2` of a producer is updated only when the value is sent, so a node's own
  values must keep coming from `Vars[]`.
- `VARS2_Init()` sets `Valid = 1`, so a value never received looks valid. Old
  `Vars[]` start invalid, and some are initialised locally (TECHM `APP.c:208`).
- `VAR_GetVariable()` has no timeout today. Many callers ignore `invalid` or
  pass `NULL`, so for them a timeout changes nothing.
- All external reads in Part B are in `VarMeta` and on CAN.
- IOBOARD_U and IOBOARD_D read no remote variables: they are not part of this
  task.

## Rules

- Live network: agents never flash live nodes. Martin does it.
- **Build only the node being changed.** The post-build step writes
  `Deployment/vXXX.YY/` by `ASW_VERSION`, so a build of any other node would
  overwrite its deployed artifacts.
- Never overwrite or revert Martin's uncommitted files. `git status` first;
  ask about anything unexpected.
- No change of module control logic. If a module would behave badly with the
  new timeouts, report it and stop before implementing that node.
- Commits: small and logical on the feature branch. Do not merge.

## Part A — shared layer (`UHA_COMMON`)

Branch `feature/vars2_read_layer` from `main`. No node build, no version bump.

1. `VARS.c/.h`:
   ```c
   static uint8_t mReadVars2;

   void VAR_EnableVars2Read(void) { mReadVars2 = 1; }

   /* always reads the old Vars[]; the current body of VAR_GetVariable() */
   int16_t VAR_GetVariableLegacy(uint16_t varId, uint16_t* invalid);

   int16_t VAR_GetVariable(uint16_t varId, uint16_t* invalid)
   {
     int16_t value;
     if (mReadVars2 && VARS2_GetLegacy(varId, &value, invalid))
     {
       return value;
     }
     return VAR_GetVariableLegacy(varId, invalid);
   }
   ```
   `VAR_GetVariablePointer()` stays unchanged (not used by application code).
2. `VARS2.c/.h`: add a `Received` flag to `sVars2State`, 0 after `VARS2_Init()`,
   set to 1 in `VARS2_ProcessFrame()` only. Add:
   ```c
   /* returns 0 = caller uses the old Vars[]:
    * unknown id, own variable, or nothing received yet */
   uint8_t VARS2_GetLegacy(uint16_t legacyVarId, int16_t* value, uint16_t* invalid);
   ```
   - Id lookup as in `VARS2_SendLegacy()`.
   - Return 0 if `meta->Producer == mThisNodeId` or `Received == 0`. Falling back
     to `Vars[]` keeps today's startup behaviour exactly.
   - `VT_FLOAT`: value = physical * `LegacyScale`, rounded to nearest (no libm).
     Other types: integer value of the 4 bytes, as `BuildLegacyFrame()` writes it.
   - If `!VARS2_IsValid(meta->Id)`: `*invalid |= INVALID_FLAG`, `invalid` may be
     `NULL`. Still return the last value, like `VAR_GetVariable()` does today.
3. `BuildLegacyFrame()` in `VARS2.c` and `SendVariable()` in `COM.c` must use
   `VAR_GetVariableLegacy()`: transmission always publishes `Vars[]`.
4. PC unit test (extend `VarDefs/tests`): layer disabled = old behaviour; own
   variable, unknown id and nothing received use `Vars[]`; round trip of every
   `legacy_scale` incl. 0.1 and negative values equals the sent raw value;
   invalid flag; timeout; `invalid == NULL`.
5. Independent reviewer subagent. Report to Martin.

## Part B — one node

Branch `feature/vars2_read_<node>` from `main` in the node's repo.
`UHA_COMMON` must contain merged Part A.

1. **Analysis first.** For each external read in the table, state what the code
   does with `invalid` and what happens after a timeout (producer offline,
   9 s or 3 s, value frozen and flagged invalid). Check the node's `SCOM.c`
   scan list for remote variables too. If behaviour would get worse, stop and
   ask Martin.
2. Call `VAR_EnableVars2Read();` in `APP.c` right after `COM_Init(THIS_NODE);`.
3. Bump `ASW_VERSION` by 1 **in this project only**, as its own commit, and
   check that the new `Deployment/` folder does not exist.
4. Build this node only. Add the entry at the top of `Deployment/CHANGELOG.md`
   (date, task, branch, CRC from `info.txt`, what changed). Commit artifacts and
   changelog.
5. Independent reviewer subagent.
6. Report: analysis, version + CRC, flash/RAM before → after, and what Martin
   should check after flashing.

| Order | Node (repo) | External reads | Notes |
|---|---|---|---|
| 1 | RECU (`ASW_RECU_NUCLEO`) | `RECON.c:92-95`: `TEMP_KIDROOM`, `TEMP_DOWNSTAIRS`, `TEMP_OUTSIDE`, `BAT_SOC` | First sync `UhaCommon` (`VARS`, `VARS2`, `COM`) with `UHA_COMMON`, keep identical, separate commit |
| 2 | ELECON_D (`ASW_ELECON_D`) | `APP.c:451,458`: `MPPT_SOLAR_VOLTAGE_V100`; `LEMON.c:438`: `BAT_SOC` | `LEMON.c` passes `NULL` as `invalid` |
| 3 | ELECON (`ASW_ELECON`) | `AC.c:97`: `POW_KITCHEN_W`; `GEMON.c`: `STRG2_*`, `LLOAD2_*`, `LPV2_*` (11 variables) | See below |
| 4 | TECHM (`ASW_TECHM`) | `ELHEATER.c:107,142-146`: `BAT_VOLTAGE_V10`, `BAT_SOC`, `CHARGING_A10`, `BAT_CURRENT_A10`, `MPPT_SOLAR_VOLTAGE_V100`; `TEMPCON.c:109,112,130`: `TEMP_DOWNSTAIRS`, `TEMP_KIDROOM`, `TEMP_OUTSIDE` | See below |

**ELECON:** `SendVariable()` in `SCOM.c` must use `VAR_GetVariableLegacy()`, so
old `0x221` keeps carrying `Vars[]` and the old-vs-new comparison in UhaGatePy
stays meaningful. `GEMON.c` is a known weak spot (invalid inputs): the analysis
must cover it in detail.

**TECHM:** last and most critical (heating, heater load). `APP.c:208` initialises
`VAR_BAT_VOLTAGE_V10` to 520; the fallback in `VARS2_GetLegacy()` preserves this.
Battery protection must not get worse if ELECON goes offline: cover it in the
analysis in detail.
