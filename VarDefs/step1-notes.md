# VARS2 step 1 — working notes

## Study findings

- `COM.c` / `COM.h` match the brief exactly: `SendVariable(id)` (static, called
  by `COM_Update_10ms()`'s stream list and by `COM_SendVariable()`) builds the
  old `0x110+node` frame from `VAR_GetVariable(id, &invalid)`. Hook point for
  `VARS2_SendLegacy(id)` is right after `MCAN_SendFrame(CMD_VAR_VALUE + mNodeId, ...)`
  in `SendVariable()`.
- `COM_GetRxMessage()` is a 1:1 wrapper of `MCAN_GetRxMessage()`. Every `APP.c`
  (ELECON, ELECON_D, TECHM, IOBOARD_D, IOBOARD_U, RECU_NUCLEO, EVSE, ELECON_TEST)
  calls it as `while (1 == COM_GetRxMessage(&rmsg)) { ProcessMessage(&rmsg); }`.
  Filtering VARS2 frames inside `COM_GetRxMessage()` with an internal loop
  (consume+forward to `VARS2_ProcessFrame()` while the fetched frame is
  `0x180..0x18F`, keep fetching until a non-VARS2 frame or empty buffer, return
  that) preserves this call pattern exactly — no `APP.c` change needed.
- `MCAN.c` FIFO1 filter already passes `0x000-0x6FF` (confirmed in code), so
  `0x180+node` frames arrive without any filter change.
- `Vars[]` (old values) is **not** `extern`'d in `VARS.h` — only accessible via
  `VAR_GetVariable()/VAR_SetVariable()/VAR_GetVariablePointer()`. `VARS2_SendLegacy()`
  uses `VAR_GetVariable(legacyVarId, &invalid)` (identical to what `SendVariable()`
  already does) instead of touching `Vars[]` directly — avoids any `VARS.h` change.
- VARS2 ids equal the old `VAR_*` ids for every variable that has a `legacy:`
  mapping in `uha_vars.yaml` (by construction) — so `VarIdToIndex[legacyVarId]`
  already gives the right row; no separate legacy->VARS2-id table is needed in
  step 1.

## Build-config finding (important, affects step 1 scope)

`UHA_COMMON/Inc`,`Src` are linked into each ASW project **file-by-file** in
`.project` (`<link><name>Common/Src/COM.c</name><locationURI>PARENT-1-PROJECT_LOC/UHA_COMMON/Src/COM.c</locationURI>`),
not as one folder link. Confirmed for ASW_ELECON, ASW_ELECON_D, ASW_TECHM,
ASW_IOBOARD_D, ASW_IOBOARD_U (`grep -c "<link>" *.project` all > 0). New files
(`uha_vars_gen.h/.c`, `VARS2_types.h`, `VARS2.h/.c`) need a new `<link>` entry
added to **each** project's `.project`, or STM32CubeIDE will not see them at
all — the CDT indexer/build only compiles files physically present in the
project tree.

**ASW_RECU_NUCLEO does not use linked resources.** Its `.project` has zero
`<link>` entries; it carries a physical, already-drifted **copy** of the
UHA_COMMON sources under `ASW_RECU_NUCLEO/UhaCommon/Src`+`Inc` (e.g. its
`COM.c`/`COM.h`/`MCAN.c`/`MCAN.h` are dated Feb 2026, its `VARS.c`/`VARS.h`
Sep 2025 — not the current `UHA_COMMON` files, byte-for-byte different).
Editing `UHA_COMMON/Src/COM.c` has **no effect** on this node unless its local
copy is updated too. This needs a decision from Martin (flagged, not
resolved here): either (a) also hand-patch
`ASW_RECU_NUCLEO/UhaCommon/{Src,Inc}` with the same COM.c hook + new VARS2
files, knowing it is otherwise out of sync with UHA_COMMON already, or
(b) convert it to real linked resources first (separate task, out of scope
for step 1 per the brief's "no change outside COM.c / new files... ask
Martin"). Proceeding with (a) — minimal, mirrors the hook into the local
copy — unless told otherwise, and reporting the pre-existing drift as a
separate open point.

`ASW_EVSE` and `ASW_ELECON_TEST` are out of the "build all six nodes" list
(EVSE is not on the bus; ELECON_TEST is the desk-test board, built from the
same ELECON sources/links). `ASW_ELECON_TEST` also calls `COM_GetRxMessage()`
in the same pattern — it picks up the UHA_COMMON hook via its own linked
resources and is used for the desk test in step 6, not part of the 6-node
headless build list in step 4.

## Plan

1. (done) Move `uha_vars_gen.h` -> `UHA_COMMON/Inc`, `uha_vars_gen.c` ->
   `UHA_COMMON/Src`, `VARS2_types.h` -> `UHA_COMMON/Inc`. Generator defaults
   updated, `-o` kept as a single-dir override. `compile_check.py`, `README.md`
   updated. `VarDefs/generated/` removed.
2. (done) `UHA_COMMON/Inc/VARS2.h`, `UHA_COMMON/Src/VARS2.c` hand-written,
   per spec sec.4 / brief sec.2.2-2.3.
3. Parallel:
   - (a) `COM.c` hooks (`VARS2_Init()` from `COM_Init()`, `VARS2_SendLegacy()`
     from `SendVariable()`, VARS2 frame filtering in `COM_GetRxMessage()`) +
     add the missing `<link>` entries to all six projects' `.project`
     (+ `ASW_RECU_NUCLEO` local copy, per the finding above) + verify include
     paths / `.cproject` source entries don't exclude the new files.
   - (b) PC unit test of `VARS2.c` (gcc, stub HAL) per brief sec.4 item 3.
4. Build all six nodes headless, compare flash/RAM before/after, zero new
   warnings.
5. Independent reviewer subagent.
6. Desk test (ELECON_TEST board) — needs Martin's hands-on execution on the
   physical CAN bus; not something this environment can drive.
7. Small logical commits on `vars2-step1`, no merge.

## Open points for Martin

- ASW_RECU_NUCLEO drift (see above) — confirm the chosen approach once this
  step's diff is ready for review.
- Desk test (step 6) and live-node flash order recommendation: see final
  report.
