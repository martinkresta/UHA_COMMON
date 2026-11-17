# Step 3c — phase 1: proposed mapping (inverters, RECU)

Brief: `cmd_inverters_recu.md`. Nothing is implemented yet; this is the proposal for Martin's OK.
Studied: `ASW_ELECON/Core/Src/AC.c`, `ASW_ELECON_D/Core/Src/AC.c` + `LEMON.c`, `ASW_RECU_NUCLEO/Core/Src/RECON.c`,
the `0x130` / `0x140` senders in TECHM (`HEATING.c`, `TEMPCON.c`, `APP.c`) and the gesture paths in `APP.c`.

## 1. How the inverters behave today

State machine (same code in ELECON and ELECON_D), `AcStateMachine()` every 1 s:

| State | Meaning | Next |
|---|---|---|
| `acs_Off` | relay off | only by gesture / remote request / 300 W auto start |
| `acs_On` | on without auto off | `KeepOn` once `EnableAutoOff` = 1 |
| `acs_OnKeepSetting` | first 5 s after a gesture "on"; each further gesture adds 2 h | `KeepOn` |
| `acs_KeepOn` | `OnTimer` counts down | `AutoOff` at 0 |
| `acs_AutoOff` | waits for power < `AutoOfPower` | relay off → `Off` (or back to `On` if auto off disabled) |

Gesture (`ButtonGesture()`) is a toggle: `Off` → on (`OnTimer = MinOnTime`, 5 s `OnKeepSetting`); `On` / `KeepOn` /
`AutoOff` → relay off at once; **`OnKeepSetting` → +2 h, not off**.
`RemoteRequest()` (CAN `0x130`, TECHM): ON → `RemoteRequests++`, `OnTimer = max(OnTimer, keepOn)`, `KeepOn`, relay on;
OFF → `RemoteRequests--`, at 0 → `AutoOff` (switches off only when the load is low). The gesture "off" ignores
`RemoteRequests`.

| Inverter | Node | Gesture today | Other owners |
|---|---|---|---|
| 300 W (REL1) | ELECON | none | boots `On`; auto start every full hour and on kitchen power > 50 W; auto off only Nov–Feb (fridge!); TECHM `0x130` type 0 keeps it on while heating (`0xFFFF` s) |
| 3 kW (REL2) | ELECON | `GEST_AC3KW_TOGGLE` (IOBOARD_D / U) | min on 15 min, auto off < 6 W |
| 5 kW (REL3) | ELECON | `GEST_AC5KW_TOGGLE` | min on 5 min, auto off < 15 W |
| Axpert (REL1) | ELECON_D | `GEST_ACAXPERT_TOGGLE` (local buttons 0+1, IOBOARD_D) | min on 5 min, auto off < 20 W; **LEMON `AC_ForceOff()` every 1 s while `mLC_AllLoads.State == 0`** |
| Victron (REL3) | ELECON_D | none | **owned by LEMON**: every 1 s `RemoteRequest(ON, 1 h)` while it is `Off` and all loads are allowed, `AC_ForceOff()` otherwise; not in `AC_Update_1s()` (state stays `KeepOn`) |

## 2. Proposed mapping

`type: bool`, 1 = on, 0 = off, `category: inverter`. Handler in `APP_OnCommand()` of the node, one small new function
in `AC.c` per node: `AC_Command(eDCACType type, uint8_t on)` (module setter, as 3b), built only from the existing
branches:

| Command | 1 (on) | 0 (off) |
|---|---|---|
| `CMD_AC_3KW` (401), `CMD_AC_5KW` (402) | only in `Off`: `ButtonGesture()` → on as a person at the switch (min on time, then auto off by power). Already on: ignored (no +2 h, no timer restart) | in `On` / `KeepOn` / `AutoOff`: `ButtonGesture()` → off at once (as the switch). In `OnKeepSetting` (first 5 s): the off branch of `ButtonGesture()` (relay off, `Off`) instead of +2 h. Already `Off`: ignored |
| `CMD_AC_AXPERT` (701) | same as 3 kW / 5 kW | same as 3 kW / 5 kW. LEMON's `AC_ForceOff()` stays on top: an "on" while all loads are blocked is switched off again within 1 s |
| `CMD_AC_300W` (403) | only in `Off`: the same as the hourly fridge start (`OnTimer >= 300`, `KeepOn`, relay on) — then the normal auto off (Nov–Feb only) | **proposal: ignored** (see risks). Alternative if you want it: only when `RemoteRequests == 0` (TECHM is not heating), as the gesture "off" |
| `CMD_AC_VICTRON` | **proposal: no command** — LEMON re-enables it within 1 s after any off and forces it off when loads are blocked; a command could not hold either state | — |

Nothing calls `RemoteRequest()` from the command path, so TECHM's `RemoteRequests` count is never changed by the RPi.
ELECON commands (target 4) arrive through the local path `SCOM.c` → `CMD_ProcessFrame()` → `APP_OnCommand()`
(checked in 3a: host test `test_cmd_fwd`, target 4 never reaches CAN); ELECON's `APP_OnCommand()` is the empty
`switch` from 3a and gets the cases.

Feedback: the existing states `ST_AC_300W/3KW/5KW` (ELECON) and `ST_AC_AXPERT` (ELECON_D), enum `InverterState`.

## 3. RECU

- `CMD_RECU_MODE` (501), `type: enum`, `enum: RecuRemoteMode`, state `ST_RECU_REMOTE_MODE`, `category: ventilation`.
- Same function as the `0x140` path: `RECON_RemoteRequest(mode, MAX_REMOTE_REQUEST_DURATION)`; the maximum is
  **1000 s** (`RECON.h`, verified, clamped inside `RECON_RemoteRequest()`); `Auto` → `RECON_RemoteRequest(Auto, 0)`
  (cancels). Enum value clamped to 0..4 before the call.
- Protections stay on top: invalid inputs and battery off (`soc < LOW_SOC_OFF_THRESHOLD`) return before the remote
  mode is applied; the antifreeze stop is applied after it. The low-SOC eco limit is **not** applied to a remote mode
  (same as today for TECHM's requests).
- `ASW_RECU_NUCLEO/UhaCommon/` no longer exists (removed in VARS2 Part C, RECU links `UHA_COMMON` like every node),
  so there is nothing to sync; RECU already builds with `CMD.c` since 3a.

## 4. Risks / points for your decision

1. **300 W off** powers off the fridge outside the hourly restart (outside Nov–Feb it comes back at the next full
   hour, in Nov–Feb when the next auto start hits). Hence the proposal: 300 W gets "on" only.
2. **RECU shares one remote mode with TECHM.** A UI `Auto` while the stove door is open cancels TECHM's
   max overpressure (600 s, smoke protection); a TECHM `Auto` (door closed / heating state change) also cancels a
   UI mode. Options: (a) accept, last writer wins (simplest, as today between TECHM's own requests); (b) RECON ignores
   UI commands while a TECHM request is active (needs a source flag in RECON — small new logic). I recommend (b) for
   `Auto` only: a UI `Auto` does not cancel a running TECHM request; other UI modes overwrite as today.
3. **Axpert state at boot (pre-existing):** `AC_Init()` in ELECON_D switches REL1 **on** but leaves the state
   `acs_Off`, so after a reset the Axpert runs, `ST_AC_AXPERT` shows `Off`, auto off never happens and the first
   gesture keeps it on (Off branch) instead of switching it off. With the mapping above, an "off" command after a reset
   would be ignored for the same reason. Fix candidate (one line, your call): init state `acs_On` (then the state
   machine goes `KeepOn` → `AutoOff` → off at low load) — this changes boot behaviour, so not part of 3c unless you say so.
   Victron: same pattern (relay on, state `Off`), but LEMON takes it over in the first second.
4. **On while on is ignored** (no "+2 h" from the UI). If a "keep on 2 h" button is wanted later, it would be a
   separate command (`uint32` hours) using the `OnKeepSetting` branch.
5. Pre-existing, outside 3c: `COM_SendACRemoteRequest()` writes `data[5] = 0` twice (overwrites the low byte of
   `keepOnTime`, `0xFFFF` arrives as 65280 s) and never sets `data[6]` (stack garbage, unused by the receivers).
   `COM_SendRecuRemoteRequest()` has the same duplicated `data[5]` line (harmless there, `data[6]` uninitialised).

Please confirm or change: the table in section 2 (especially 300 W "on" only and no Victron command), RECU option
(a)/(b), and whether the Axpert init fix goes in.
