# Commands: inverters and RECU mode (step 3c)

Precondition: 3b done and flashed. Spec `UHA_COMMON/VarDefs/CMD_spec.md`, pattern from 3a/3b.
Lightweight mode, **Martin flashes**, version bump + CHANGELOG per node, branch `feature/cmd-3c`.

**Two-phase for this step** (inverters power the house — a wrong mapping can switch it off):
1. Study `AC.c` (ELECON and ELECON_D) and `RECON.c` and **write the proposed mapping into the report first**
   (what each command value does, which existing function is called, how it interacts with auto-off,
   `RemoteRequests`, TECHM's requests and `AC_ForceOff()`). **Stop and wait for Martin's OK.**
2. Then implement exactly the agreed mapping.

## Inverters

| Command | Node | Existing state |
|---|---|---|
| `CMD_AC_300W`, `CMD_AC_3KW`, `CMD_AC_5KW` (ids 4xx) | ELECON | `ST_AC_300W/3KW/5KW` (`ACState`) |
| `CMD_AC_AXPERT`, `CMD_AC_VICTRON` (ids 7xx) | ELECON_D | `ST_AC_AXPERT/VICTRON` |

- Starting proposal (to confirm in phase 1): `type: bool`; 1 = on, 0 = off, **with the same effect as the
  physical button / gesture** for that inverter (so auto-off, keep-on and the state machine behave exactly as today
  for a person at the switch). If the gesture is a toggle only, build on/off from the current state; never
  bypass `AC_ForceOff()` or any protection.
- ELECON commands (target 4) arrive through the local path in `SCOM.c` (`CMD_ProcessFrame()` directly) — check
  this path end to end.

## RECU

- `CMD_RECU_MODE` (ids 5xx): `type: enum`, `enum: RecuRemoteMode`, state `ST_RECU_REMOTE_MODE`.
- Proposal: call the same function the `0x140` receive path calls today, with the **maximum duration RECON accepts**
  (1000 s per `CLAUDE.md` — verify); `Auto` cancels. RECON's battery protection and limits stay on top.
- `ASW_RECU_NUCLEO/UhaCommon/`: sync the command-channel files from `UHA_COMMON` first (must be identical).

YAML: `category: inverter` for the five inverter commands, `category: ventilation` for `CMD_RECU_MODE`.

## Not in this step

`0x130` / `0x140` stay. They are used **node to node by TECHM** (`HEATING.c`, `TEMPCON.c`, `APP.c`: keep the
300 W inverter on while heating, RECU overpressure when the stove door opens / cleaning mode), not only by
gestures. Moving those to the command channel needs node-to-node commands and touches TECHM — a separate decision.

## Report

Phase 1: the mapping table + risks. Phase 2: versions, commands added, files, RAM/flash delta, test notes.
