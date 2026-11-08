# Task: VARS2 step 2 — ELECON → RPi bridge: VARS2 tunnel over RPISERP

## 1. Context

Read first: `UHA_FW/CLAUDE.md`, `UHA_COMMON/CLAUDE.md`, `ASW_ELECON/CLAUDE.md`,
`UHA_COMMON/VarDefs/VARS2_spec.md` — **especially §7 (the tunnel) — it is the
contract with the RPi side**, and `UHA_COMMON/VarDefs/step1-notes.md`.

**Prerequisite:** step 1 (`VARS2.c`, dual transmission, branch `vars2-step1` in
`UHA_COMMON`) is finished. Build on top of it.

The RPi is not on CAN. ELECON (node 4) forwards data over USB-serial using
RPISERP (`ASW_ELECON/Core/Src/SCOM.c` = scan list + message handling,
`RPISERP.c` = framing + DMA). Today it sends old variables as packet `0x221`
from a scan list (max 1 frame per 10 ms), log messages `0x620`, and receives
RTC sync `0x051` from the RPi.

**Goal:** ELECON additionally sends the **VARS2 tunnel** exactly as defined in
spec §7.2–§7.3. A separate agent builds the RPi side (UhaGatePy) against the
same spec — do not change the format; if the spec is unclear, ask Martin.

## 2. Requirements

1. **Tunnel of received VARS2 frames:** every VARS2 frame ELECON receives
   from CAN is forwarded immediately (RPISERP packet id = CAN COB-ID
   `0x180 + producer`, data = the 8 CAN data bytes unchanged).
2. **Tunnel of ELECON's own VARS2 frames** (those `VARS2_SendLegacy` sends on
   CAN): forwarded with packet id `0x184`.
3. **ELECON-local variables with `on_can: false`** (`VarMeta`: producer
   `NODEID_ELECON`, `VMF_ON_CAN` not set): sent over the tunnel only, packet
   id `0x184`, period `VarMeta.PeriodMs`, value + validity converted from
   `Vars[]` like `VARS2_SendLegacy` (reuse its conversion code — do not
   duplicate it).
4. **Old traffic unchanged:** scan list `0x221` (content, periods, 1 per
   10 ms), `0x620` log forwarding, RTC and all RPi → ELECON handling.
5. **Serial throughput** (spec §7.4): today `RSP_TransmitFromFifo()` sends one
   frame per 10 ms and the TX FIFO holds 10 frames. Change `RPISERP.c` so the
   next frame starts on DMA TX-complete (or equivalent), enlarge the TX FIFO
   (e.g. 64), target ≥ 300 frames/s without drops. **Count FIFO overflows**
   (counter readable via a debug variable or log message).

6. **Two fixes from the step 1 review** (`UHA_COMMON/VarDefs/step1-notes.md`),
   both in the VARS2 code:
   - `VARS2_SendLegacy()` also updates the producer's own state
     (`mState[idx].Valid = valid`, `RxTick = HAL_GetTick()`), so
     `VARS2_IsValid()` on a node's own value does not report a timeout.
   - Accept a VARS2 frame only if DLC == 8 and it is not an RTR frame
     (check in `COM_GetRxMessage()` before `VARS2_ProcessFrame()`, or pass
     the DLC in); a rejected `0x18x` frame is dropped, not returned to the
     application. Cover both in the unit test.

### Design hints

- In `UHA_COMMON/VARS2.c` add a generic hook, e.g.
  `VARS2_SetFrameHook(void (*hook)(uint16_t cobId, const uint8_t* data8))`,
  called for every VARS2 frame received (RX path) and sent (TX path).
  Default: no hook (all other nodes unaffected). ELECON registers a forwarder
  to `RSP_Send()` in its `SCOM_Init()`.
- Requirement 3: a periodic function in `VARS2.c`
  (e.g. `VARS2_ForEachLocalDue(...)` or `VARS2_UpdateLocal_10ms(cb)`) driven
  by `VarMeta.PeriodMs`, called from ELECON's `SCOM_Update_10ms()`.
- Keep `RPISERP.c` changes minimal and interrupt-safe (FIFO accessed from
  main loop and TX-complete IRQ).

## 3. Constraints

- Repos (feature branch `feature/elecon_vars2_tunel`, from `main`, or `master`
  in `ASW_TECHM`, in each):
  - code changes: `UHA_COMMON`, `ASW_ELECON`, `ASW_RECU_NUCLEO` (its local
    copy of the fixed files);
  - version, `Deployment/` and changelog only: `ASW_ELECON_D`,
    `ASW_IOBOARD_D`, `ASW_IOBOARD_U`, `ASW_TECHM`.
- `git status` first in all of them; ask about unexpected uncommitted changes.
  Known and expected, leave them uncommitted and untouched:
  `.cproject` in `ASW_ELECON_D`, `ASW_IOBOARD_D/Deployment/v001.00/*`.
  Never overwrite or revert uncommitted files of Martin's.
- Change only: `UHA_COMMON/VARS2.c/.h` (hook + local publishing + fixes),
  `UHA_COMMON/COM.c` (DLC/RTR check only), the same files in the
  `ASW_RECU_NUCLEO/UhaCommon` copy (keep it identical to `UHA_COMMON`),
  `ASW_ELECON/Core/Src/SCOM.c`, `RPISERP.c` (+ headers), `Core/Inc/APP_INFO.h`
  and `Deployment/` in the six projects. Anything else → ask.
- No change of other nodes' behaviour apart from the two review fixes; all
  six nodes must still build (UHA_COMMON is shared).
- **Version bump is mandatory.** The post-build step writes artifacts to
  `Deployment/vXXX.YY/` according to `ASW_VERSION` in `Core/Inc/APP_INFO.h`,
  so a build on an existing version overwrites artifacts that may already be
  flashed. Before the first build, bump `ASW_VERSION` by 1 in every project
  whose firmware changes and check that the new folder does not exist yet.
  Shared code changes here, so all six change:

  | Project | Current | New |
  |---|---|---|
  | `ASW_ELECON` | v001.11 | **v001.12** |
  | `ASW_ELECON_D` | v001.16 | v001.17 |
  | `ASW_IOBOARD_D` | v001.01 | v001.02 |
  | `ASW_IOBOARD_U` | v001.01 | v001.02 |
  | `ASW_RECU_NUCLEO` | v001.01 | v001.02 |
  | `ASW_TECHM` | v001.03 | v001.04 |

  Rebuilding the new version within this task (e.g. after review fixes) is
  fine, it is not flashed yet.
- **Changelog is mandatory.** Add an entry for the new version at the top of
  `Deployment/CHANGELOG.md` in each of the six projects, in the existing
  format (date, task, branch, CRC from `info.txt`, what changed for that
  node). The file may still be untracked: commit it with the entry.
- **No flashing of the live nodes** — Martin does it.
- Desk test: the desk board ("ELECON_TEST", project `ASW_ELECON_TEST`, runs
  as node 4 with an ELECON bootloader, see `C:\Martin\GIT\C#\UhaCanMaster\docs\bootloader-behavior.md`)
  may be flashed with the ELECON build **only on the desk bus** (CAN side
  only). The serial link is only ever connected to the RPi; the serial side
  is verified later on the live link with UhaGatePy (step 3) after Martin
  flashes ELECON.

## 4. Work plan

1. Study (spec §7, SCOM.c, RPISERP.c, VARS2.c, ELECON APP/scheduler). Write the
   plan to `UHA_COMMON/VarDefs/step2-notes.md`.
2. Implement. You may use subagents: (a) `RPISERP.c` TX throughput + FIFO,
   (b) VARS2 hook + local publishing + SCOM integration.
3. **PC-side unit test** (gcc, stub HAL) of the new code: tunnel frame
   building (packet id, payload bytes = CAN bytes), local publishing periods
   and legacy conversion, FIFO behaviour incl. overflow counting.
4. Bump `ASW_VERSION` in all six projects (table in §3, a separate commit in
   each repo), build all six nodes, write the changelog entries. ELECON
   flash/RAM before → after.
5. **Independent reviewer subagent**: checks against spec §7, IRQ safety of
   the FIFO, no change of old traffic, other nodes unaffected apart from the
   two review fixes.
6. Desk test (CAN side only): ELECON build on the desk board still sends old
   and VARS2 frames on CAN as in step 1, no regressions. Record in
   `step2-notes.md`.
7. Small logical commits on the feature branch in every repo listed in §3,
   including the new `Deployment/vXXX.YY/` artifacts and
   `Deployment/CHANGELOG.md`. Do not merge.

## 5. Report to Martin

Changes, list of new versions with CRC for flashing, build sizes, unit/desk
test results, anything in spec §7 that
needed interpretation, and what to check on the live link after flashing
ELECON (UhaGatePy test window: tunnel rates per node, 0 checksum errors,
old vs new mismatch summary).
