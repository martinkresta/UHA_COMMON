# VARS2 step 1 — working notes

## Branches

The brief assumed only `UHA_COMMON` would need a `vars2-step1` branch ("same
branch name in any other repo you have to touch (should be none)"), but the
build-config work touches six more repos (each ASW project is its own git
repo). Created `vars2-step1` in all of them, from their current branch:

| Repo | Branched from | Pre-existing local state carried onto the branch (not ours, don't touch) |
|---|---|---|
| `UHA_COMMON` | `vars2-step0` | clean |
| `ASW_ELECON` | `dev_asw` | uncommitted: `Core/Src/LEMON.c`, `Deployment/v001.10/*` |
| `ASW_ELECON_D` | `main` | clean (`lemon-soc-from-bms` was already merged into `main` at `fe12b82`, matches `origin/main` — no merge needed, confirmed before branching) |
| `ASW_TECHM` | `dev_asw` | clean |
| `ASW_IOBOARD_D` | `dev_asw` | uncommitted: `.cproject`, `.settings/stm32cubeide.project.prefs`, `Core/Src/main.c`, added `Deployment/v001.00/*` |
| `ASW_IOBOARD_U` | `dev_asw` | untracked `CLAUDE.md` only |
| `ASW_RECU_NUCLEO` | `dev_asw` | untracked `CLAUDE.md` only |

The pre-existing uncommitted/untracked files above are Martin's own
in-progress work, unrelated to VARS2 — left alone, not staged/committed by
this task (commits here only `git add` the specific files VARS2 touches).

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
`<link>` entries; it carries a physical **copy** of the UHA_COMMON sources
under `ASW_RECU_NUCLEO/UhaCommon/Src`+`Inc` (a git subtree, squashed in a long
time ago, never updated since). Of its ~17 files, 5 had drifted from the
current `UHA_COMMON`: `COM.c`, `COM.h`, `MCAN.c`, `MCAN.h`, `VARS.h` (missing
`VAR_*` ids added since, e.g. BMS3/STRG2/LVDC) — the rest (ADC, BMS, ELM,
GEST, LOG, METEO, OW, PID, RTC, SENSIRION, TEMP, UHAMON, UI, circbuf,
watchdog) are byte-identical already.

**Martin's decision (asked 2026-11-01): sync now.** Replace those 5 files in
`ASW_RECU_NUCLEO/UhaCommon/{Inc,Src}` with the current `UHA_COMMON` versions,
then add `uha_vars_gen.h/.c`, `VARS2_types.h`, `VARS2.h/.c` there too (own
copies, generated from the same `uha_vars.yaml`) and apply the same `COM.c`
hook. `ASW_RECU_NUCLEO` is its own git repo (branch `vars2-step1`, from
`dev_asw`) — commit the sync and the VARS2 addition there as separate small
commits, same as in `UHA_COMMON`.

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

## Independent review (fresh agent, did not write the code) — PASS

No blocking bugs. Confirmed: old protocol (`VARS.c/.h`, `MCAN.c/.h`, old
`0x110` path in `COM.c`) byte-identical to before in every repo except the
approved RECU_NUCLEO sync; frame layout matches spec sec.2 byte-for-byte on
both TX and RX; `COM_GetRxMessage()`'s `(StdId & 0x7F0) == 0x180` mask
collides with no other COBID in use and the consume-loop is bounded by the
RX ring buffer size; all 257 generated rows have `legacyId == id` (the
"legacy id == VARS2 id" shortcut holds, 0 mismatches); every
`COM_AddStreamedVariable`/`COM_SendVariable` call site across all ASW
projects (159 sites) resolves to a defined, `on_can` VARS2 variable (the two
exceptions are already commented-out code); RAM delta (~3.08 KB) is
negligible on every target including the smallest (L431RC, 64 KB RAM);
ASW_ELECON_D's 5 new `<link>` entries are correct and unexcluded;
RECU_NUCLEO's local copy is a byte-for-byte full match of UHA_COMMON;
generator output is deterministic (`gen_all.py` re-run changes nothing);
`run_validator_tests.py` 63/63, `compile_check.py` and `test_vars2.py` both
pass.

Two non-blocking notes, both already anticipated as step-6-not-step-1 concerns:
- **Latent, matters at step 6, not now:** `VARS2_SendLegacy()` writes the
  value into `Vars2` but does not refresh `mState[idx].RxTick`/`.Valid` for
  the producer's own variables — after `TimeoutMs` with no further send,
  `VARS2_IsValid()` on a node's own produced value would read as "timed
  out" even though it's being actively (re)sent. Nothing reads
  `VARS2_IsValid()` in step 1, so no effect now. Before step 6 (first module
  that reads `Vars2`), either have `VARS2_SendLegacy()` also refresh
  `RxTick`/set `Valid`, or give producer-local variables a different check
  (e.g. skip the timeout for `VarMeta[idx].Producer == thisNodeId`).
- **Minor, theoretical:** `VARS2_ProcessFrame()`/`COM_GetRxMessage()` don't
  check DLC or RTR on a `0x18x` frame; a malformed/RTR frame from a
  misbehaving sender would be decoded from stale buffer bytes. Not
  reachable from any node in this codebase today.

Housekeeping noted by the reviewer, not touched (out of scope / not ours):
`UHA_COMMON/VarDefs/VARS2_spec.md` has an unrelated working-tree
modification and `vars2-step2-brief.md` is untracked there already —
pre-existing, left alone.

## Build results (headless, `C:\Martin\CUBEIDE\UHA_workspace`)

All six: 0 build errors, `uha_vars_gen.o`/`VARS2.o` confirmed compiled, 0 new
compiler warnings (every warning present is pre-existing, in files VARS2
doesn't touch: OW.c, MCAN.c, TEMP.c, METEO.c, PID.c etc.). Per Martin's
request (2026-11-01), `ASW_VERSION` was bumped by 1 in every project first
so the verification build lands in a new `Deployment/vXXX.YY/` folder
instead of overwriting the released one.

| Node | Version | Flash (text+data) | RAM (data+bss) | VARS2-added (flash/RAM) |
|---|---|---|---|---|
| ELECON | v001.11 | 90488 B | 26104 B | ~6600 / ~3085 B |
| ELECON_D | v001.16 | 106692 B | 23496 B | ~6600 / ~3085 B |
| TECHM | v001.03* | 58504 B | 11120 B | ~6600 / ~3085 B |
| IOBOARD_D | v001.01 | 60800 B | 43032 B | ~6600 / ~3085 B |
| IOBOARD_U | v001.01 | 45268 B | 43200 B | ~6376 / ~3085 B |
| RECU_NUCLEO | v001.01 | 62584 B | 9944 B | ~6600 / ~3085 B |

\* TECHM's headless build initially crashed on a one-time GUI
toolchain-confirm dialog (`MCULoggerPlugin.openConfirmDialog`, "Workbench
has not been created yet") that had never been dismissed for this project
in the workspace. **Resolved 2026-11-01**: Martin built TECHM once from the
IDE himself, producing `Deployment/v001.03/` (flash 58504 B / RAM 11120 B —
exactly matching the pre-dialog-fix measurement, confirming no regression).
No source change was needed; pure one-time tooling hiccup.

**ASW_ELECON_TEST (desk-test board, same HW as ELECON/ELECON_D, not on the
real bus)** was not in the brief's "build all six" list but is the board
used for the step 6 desk test — also needed the VARS2 wiring check: it
links `Common/Inc`/`Src` as whole folders (like ELECON/TECHM/IOBOARD_D/U),
so no `.project` edit was needed, same as those. Branched `vars2-step1`
(from `master`), bumped `ASW_VERSION`. Built by Martin in the IDE (twice,
105 then 106 — both identical code, just the version he ended up on):
`Deployment/v001.06/`, flash (text+data) 82228 B, RAM (data+bss) 26432 B,
`VARS2.o`/`uha_vars_gen.o` confirmed compiled. Ready for the desk test.

Flash delta is ~5.7-6.6 KB, RAM delta ~3.0-3.1 KB on every node — the
generated `VarMeta`/`VarIdToIndex` tables (flash, 5140+764 B) plus
`Vars2`/VARS2's `mState` (RAM, 1028+2056 B), matching the PC-side
`compile_check.py` numbers. Negligible against every target's flash/RAM,
including the smallest (STM32L431RC, 256 KB/64 KB).

**Deployment/ safety, two incidents, both resolved:**
- Round 1 (before the version bump), the build fork ran ASW_ELECON's
  headless build before snapshotting `Deployment/`, overwriting Martin's own
  uncommitted `v001.10` rebuild (after his `LEMON.c` edit) with a build that
  also baked in VARS2. Fixed: reverted the 3 affected files to git HEAD
  (confirmed with Martin first). `Core/Src/LEMON.c` itself was never
  touched.
- `ASW_IOBOARD_D/Deployment/v001.00/*` shows modified vs. git HEAD (BUILD
  TIME 14.02.2026) — checked and this **predates all of this work** (it was
  already like that in the very first `git status` taken before any
  VARS2-step1 branch existed); neither build round touched it. Pre-existing
  Martin drift, left alone, not a VARS2 regression.

## Open points for Martin

- ASW_RECU_NUCLEO drift — resolved, synced (decided 2026-11-01, see above).
- TECHM headless build dialog — resolved (Martin built once in the IDE).
- Desk test (step 6, physical ELECON_TEST board on the desk CAN bus) needs
  Martin's hands-on execution — not something drivable from here.
  ASW_ELECON_TEST is built and ready (`Deployment/v001.06`).
- Live-node flash order recommendation: see final chat report.
