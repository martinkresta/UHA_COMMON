# Task: VARS2 direct access — modules work with Vars2, the old VARS API goes

The wire protocol is VARS2 only. In the firmware the old API is still the backbone: modules write and read
scaled `int16` through `VAR_SetVariable()` / `VAR_GetVariable()` and `Vars[400]`, and VARS2 converts to float
and back (`VARS2_SendLegacy()`, `VARS2_GetLegacy()`, `LegacyScale`). This task removes that layer. Modules work
with floats in physical units directly in the `Vars2` groups, with a valid flag next to every value.

**How this task is run:** Part A once, then Part B for one node at a time, then Part C. Martin says which
part/node to do and tests each node on the live system before the next one. Do only that.

## Context

Read first: `UHA_FW/CLAUDE.md`, `UHA_COMMON/CLAUDE.md`, the target project's `CLAUDE.md`,
`UHA_COMMON/VarDefs/VARS2_spec.md`, `UHA_COMMON/VarDefs/README.md` (YAML, validator, generator),
`UHA_COMMON/VarDefs/vars_dependency_map.md`, `UHA_COMMON/Src/VARS.c`, `UHA_COMMON/Src/VARS2.c`,
`UHA_COMMON/Inc/uha_vars_gen.h`.

Facts checked in the code (2026-11-14):
- Every `VAR_*` id used with a literal in the six nodes and in `UHA_COMMON` is in the YAML, so `Vars[]` can go
  completely.
- By-id users: `TEMP_AssignSensor(sensor, VAR_x, bus)`, `ELM_AddMeter(elm, pin, VAR_x)`, `SENSIRION.c`, and
  `SCOM.c` (`VAR_NETWORK_STATUS`) in IOBOARD_D, IOBOARD_U, TECHM, ELECON.
- Producer validity today is per group almost everywhere (one `validflag` per module, or always `1`).
  Exceptions: ELECON `GEMON.c` (battery, solar, load, load current) and ELECON_D `LEMON.c` (`strgvalid` vs
  `validflag`).
- BMS: drivers (`UHA_COMMON/BMS.c` in ELECON, `JKBMS.c` in ELECON_D, same API) fill an `sBMS` owned by
  `LEMON.c`, and LEMON publishes the BMS values and the storage. Published names do not match the slots
  (ELECON slot 1 = 80 Ah → `BMS3_*`, slot 2 = 220 Ah → `BMS2_*`; ELECON_D slot 2 = 251 Ah → `BMS1_*`, slot 1
  unused but also mapped to `BMS1_*`). BMS values are published only `if (Active)` with `valid = 1`, so a
  silent BMS freezes as valid; `BMS.c` never clears `Active`.
- `ASW_RECU_NUCLEO` has its own copy of the shared code in `UhaCommon/`; keep it identical. **Done in part C: RECU uses the linked UHA_COMMON again, `UhaCommon/` removed.**
- UhaGatePy does not depend on YAML group names. `uha.json` uses the YAML `legacy` names and `legacy_scale`,
  `uha2.json` uses the YAML names.

## Agreed design

**Data.** One store: the generated `Vars2`. Every variable is a typed field with value and validity:
```c
typedef struct { float    value; uint8_t valid; } sVarF32;
typedef struct { int32_t  value; uint8_t valid; } sVarI32;   /* int32, enum, bool */
typedef struct { uint32_t value; uint8_t valid; } sVarU32;

typedef struct
{
  sVarF32 Soc_pct;
  sVarF32 Voltage_V;
  /* ... */
} sVarsStorageHome;
```
Groups with an identical layout (the three packs) share one generated struct type, so one BMS driver can
publish into any of them.

**Three places, three owners.**

| What | Where | Who touches it |
|---|---|---|
| value + valid | `Vars2` (RAM, generated) | modules through group pointers; VARS2 for received values |
| description: id, type, producer, period, timeout, pointer to the field | `VarMeta` (flash, const, generated) | VARS2 only |
| protocol state: `RxTick`, `TxTick`, ... | `mState` (RAM, private in `VARS2.c`) | VARS2 only |

**Module access.** `Vars2` is visible only to `APP.c` and `VARS2.c`. Each module's `Init()` gets pointers to
its groups: non-const for groups it produces, `const` for groups it only reads. `APP.c` does the wiring, so
it shows every module's data dependencies.
```c
GEMON_Init(&Vars2.Battery, &Vars2.Solar, &Vars2.Load, &Vars2.StorageHome, &Vars2.StorageWorkshop);
```

**Producer API.**
```c
pStorage->Soc_pct.value = soc;
pStorage->Voltage_V.value = voltage;
VARS2_SetGroupValid(pStorage, valid);         /* once per update: sets .valid of every field of the group */

VarSet(&pShunt->Current_A, current, valid);   /* inline helper for a single field */
```
No other ways of writing. The exact signature of `VARS2_SetGroupValid()` (size argument or generated per-group
function) is up to you; one call per group.

**Receiver side.** VARS2 writes `value` and `valid` from the frame and clears `valid` when the variable's
timeout expires. Modules only read `pX->Field.valid`; there is no `VARS2_IsValid()` for modules.

**By id** (TEMP, ELM, SENSIRION, SCOM): a small API working through `VarMeta`, e.g.
`VARS2_SetById(eVarId id, float value, uint8_t valid)`. Configuration takes `VID_*` ids.

**TX** is driven by the YAML: VARS2 sends every variable the node produces by `period_ms` (on CAN, or through
the ELECON tunnel for `on_can: false`). `COM_AddStreamedVariable()` goes.

**Groups.** One producer and one validity per group. Ids never change: the wire format stays identical, and
UhaGatePy and the PC tool keep working.

**Concurrency.** All `Vars2` reads and writes happen in the main loop. Verify per node (CAN RX is processed
from `COM_GetRxMessage()` in the main loop; nothing in `stm32*_it.c` touches variables today). If anything
runs in an interrupt, stop and ask.

## Rules

- Live network: agents never flash. Martin does it and tests each node before the next one.
- Never overwrite or revert Martin's uncommitted files. `git status` first; ask about anything unexpected.
- Delete replaced code; no flags, no `#if 0`, no compatibility kept beyond what this task says. Git history is
  the fallback.
- Code style as in `LEMON.c`: CamelCase, unit suffix after an underscore (`Voltage_V`, `mBattRemaining_mAs`).
- Small logical commits on the feature branch. Do not merge.
- **Builds:** the post-build step writes `Deployment/vXXX.YY/` by `ASW_VERSION`. A check build of a node whose
  version is not bumped overwrites its deployed artifacts: restore them with git right after
  (`git checkout -- Deployment`, remove new untracked files there).

## Part A — shared engine and YAML (`UHA_COMMON`)

Branch `feature/vars2_direct_access` from `main`. No version bump, nothing to flash: the nodes get Part A
with their own Part B.

1. **YAML review — proposal first, then stop.** Write a table of all groups and variables with the proposed
   new group and field names (ids unchanged) to `VarDefs/direct-access-notes.md` and wait for Martin's
   approval before changing the YAML. Agreed so far:
   - Packs: `Bms1` → `PackWorkshop251Ah`, `Bms2` → `PackHome220Ah`, `Bms3` → `PackHome80Ah` (one shared struct
     type).
   - Storages: `Storage1` → `StorageHome`, `Storage2` → `StorageWorkshop`.
   - `GlobalEnergy` split into `Battery`, `Solar`, `Load` along GEMON's validity groups; `CHARGING_A10` moves
     to `Mppt`.
   - Split any other group whose fields have more than one producer or validity source (e.g. the PV and
     load fields next to the storage fields).
   - Field names in LEMON style with unit suffix, no repeated group prefix (`StorageHome.Soc_pct`, not
     `Strg1Soc`).
2. **Generator:** typed `{value, valid}` fields, shared struct types for identical groups, `VarMeta` pointing
   to the field. Keep the `VID_*` ids. Validator: new rules where needed (one producer per group).
3. **Engine (`VARS2.c/.h`):** receive into `{value, valid}`, timeout clears `valid`, YAML-driven TX of own
   variables (CAN + tunnel), `VARS2_SetGroupValid()`, `VarSet()`, by-id API, tunnel hook unchanged.
   `mState` keeps only protocol state.
4. **Temporary compatibility for unconverted node code** (removed in Part C): `VAR_SetVariable()` and
   `VAR_GetVariable()` become thin wrappers over `Vars2` via `VarMeta` (`LegacyScale` conversion exactly as
   today); `Vars[]` is deleted; `COM_AddStreamedVariable()` becomes a no-op. A wrapper write to a variable this
   node does not produce is ignored; list every such call per node (known: TECHM `APP.c` writes
   `VAR_BAT_VOLTAGE_V10` = 520 at init). Check per node that the YAML `period_ms` matches today's
   `COM_AddStreamedVariable()` periods and list differences.
5. **Shared modules** to the new API: `METEO.c` (group pointer), `TEMP.c`, `ELM.c`, `SENSIRION.c` (by id).
   `BMS.c` (ELECON only) gets its publish function in ELECON's Part B.
6. **Tests (PC, gcc):** frames on CAN identical to today for the same inputs (all `legacy_scale` values,
   negative values, every type); timeout clears `valid`; `VARS2_SetGroupValid()`; by-id API; wrapper round
   trip; tunnel frames unchanged.
7. Check build of all six nodes, then restore their `Deployment/` (see Rules).
8. Check what reads the YAML names: UhaGatePy (`uha2.json`, status pages; `deploy.sh` copies the YAML) and
   the PC tool (`C:\Martin\GIT\C#\UhaCanMaster`). Report what Martin has to redeploy; do not change them.
9. Independent reviewer subagent. Report to Martin.

## Part B — one node

Branch `feature/vars2_direct_access` from `main` in the node's repo. `UHA_COMMON` must contain merged Part A.

1. **Inventory** in `VarDefs/direct-access-notes.md` (node section): every old API call, classified as
   produced / read / by id.
2. **Wiring:** each module's `Init()` gets its group pointers (produced = non-const, read = `const`); `APP.c`
   passes them.
3. **Producers:** write `.value` in physical units, set validity with `VARS2_SetGroupValid()` (or `VarSet()`
   for single fields). Remove the scaling math.
4. **Readers:** read `.value` and `.valid`. Keep each module's existing handling of invalid inputs.
5. **Constants and thresholds** move to physical units (e.g. `520` V×10 → `52.0f`). List **every** changed
   constant with old and new value in the notes; the reviewer re-checks each one.
6. **By id:** configuration uses `VID_*`.
7. **Done when** the node has no `VAR_` call, no `VARS.h` include and no `COM_AddStreamedVariable()`.
8. Remove dead code you find on the way only if it is clearly unused; list it.
9. Bump `ASW_VERSION` by 1 in this project only (own commit, check the new `Deployment/` folder does not
   exist), build this node only, add the `Deployment/CHANGELOG.md` entry, commit artifacts and changelog.
10. Independent reviewer subagent: constants, validity handling, const-correctness, nothing written to a
    group the module does not produce.
11. Report: changes, constant list, version + CRC, flash/RAM before → after, and what Martin checks after
    flashing: this node's values in the PC tool and on the web are the same as before, validity behaves as
    before (e.g. invalid when a producer stops), control behaves as before.

| Order | Node (repo) | Notes |
|---|---|---|
| 1 | IOBOARD_U (`ASW_IOBOARD_U`) | Smallest. `SCOM.c` `NETWORK_STATUS` by id. First test of the engine on the live bus. |
| 2 | IOBOARD_D (`ASW_IOBOARD_D`) | `ElMeters` via `ELM.c` (by id), `APP.c` reads meters. |
| 3 | RECU (`ASW_RECU_NUCLEO`) | First sync `UhaCommon/` with `UHA_COMMON` (separate commit). `RECON.c` reads `Battery` and `Temperatures`; keep its invalid-input fallback. |
| 4 | ELECON_D (`ASW_ELECON_D`) | See below. |
| 5 | TECHM (`ASW_TECHM`) | See below. |
| 6 | ELECON (`ASW_ELECON`) | See below. May be split into two sessions (energy modules first). |

**ELECON_D:** `LEMON.c` gets a concrete structure: one pack `PackWorkshop251Ah`, the unused slot removed.
`JKBMS.c` publishes `PackWorkshop251Ah` itself through its group pointer, invalid when the BMS stops
responding (it has `InvalidateData()`; check it is called on a timeout). LEMON writes only `StorageWorkshop`
(and the groups split from it in Part A). `AXPERT.c` writes `Axpert` with one group validity. LEMON reads
`BAT_SOC` with `invalid = NULL` today (ignores validity): report what it should do, do not change the logic.

**TECHM:** heating and water heater control (`ELHEATER.c`, `HEATING.c`, `TEMPCON.c`, `WM.c`, `PTCTEMP.c`,
`BOILER_AIR.c`). The heater switches off on invalid inputs (`eElh_InvalidInputs`); this must not get weaker.
Remove the `APP.c` init write of `VAR_BAT_VOLTAGE_V10` = 520: an unreceived value is invalid. Constant list
is critical here.

**ELECON:** `LEMON.c` gets a concrete structure with `PackHome80Ah` (UART3) and `PackHome220Ah` (UART2).
`BMS.c` publishes its pack group itself and must mark it invalid when the BMS stops responding (today it never
clears `Active`). LEMON writes only `StorageHome` (and split groups). `GEMON.c` writes `Battery`, `Solar`,
`Load` keeping the v001.15 behaviour (invalid inputs → group invalid, last valid values kept). `MPPT.c`
writes `Mppt` incl. `Charging_A`. `SCOM.c` `NETWORK_STATUS` by id. `AC.c` reads `ElMeters`. Tunnel unchanged.

## Part C — remove the old layer (`UHA_COMMON`)

After the last node. Branch `feature/vars2_direct_access_cleanup` from `main`.

1. Delete `VARS.c/.h`, the wrappers, `COM_AddStreamedVariable()`, `VAR_GetVariablePointer()`, any remaining
   `Legacy` functions and names in `VARS2.c/.h`, `LegacyScale` / `LegacyId` from `VarMeta` and the generator.
2. YAML: `legacy` and `legacy_scale` stay (UhaGatePy uses them for `uha.json`); the generator ignores them,
   the validator keeps accepting them and drops the `VARS.h` check. Update the header comments of the YAML.
3. Sync `ASW_RECU_NUCLEO/UhaCommon`. **Done in part C: RECU uses the linked UHA_COMMON again, `UhaCommon/` removed.**
4. Check build of all six nodes, then restore their `Deployment/` (no version bump, nothing to flash: only
   unused code goes).
5. Update `VARS2_spec.md`, `VarDefs/README.md`, `CLAUDE.md` files to the new API.
6. Independent reviewer subagent. Report to Martin.
