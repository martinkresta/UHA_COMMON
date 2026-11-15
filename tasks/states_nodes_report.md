# Step 2b report — VARS2 states for IOBOARD_D, ELECON_D, ELECON, RECU

Date 2026-11-15 (evening). Brief: `UHA_COMMON/tasks/states_nodes.md`. Step 2a (IOBOARD_U) was merged and pushed
before this step started.

**Status:** firmware v002.01 for all four nodes is built and in `Deployment/v002.01/`. Nothing is flashed. The
gateway has the new YAML and the Grafana States dashboard is extended, both deployed on the Pi. All changes are
committed on branch `feature/vars2-states-2b` in UHA_COMMON, ASW_IOBOARD_D, ASW_ELECON_D, ASW_ELECON,
ASW_RECU_NUCLEO and UHARPI. **Not merged and not pushed** — that waits for your check.

## What you need to do

1. Read the points under "Please decide / check" below.
2. Flash in this order, any time, one by one (nodes with old FW ignore the new ids):
   IOBOARD_D → ELECON_D → ELECON → RECU. All are `Deployment/v002.01/*_ASW_v002.01.bin`.
3. After each flash: Grafana **UHA States** → the node's row fills in; `uha2.json` → its `ST_*` values become valid.
4. If it all looks right, tell me "merge 2b" (or merge yourself), then TECHM (2c) after your go.

## Please decide / check

1. **ELECON_D OUT4 is not "reserved".** The brief (from the old CLAUDE.md) says OUT4 is reserved and should not be
   published. But `LEMON.c` `LoadControl()` switches `OUT_HEATING_RELAY` (= OUT4) on and off by SOC. Following
   "the code wins", I published it as `ST_WS_HEATING_RELAY` (bool, id 496) and fixed the table in
   `ASW_ELECON_D/CLAUDE.md`. If you don't want it, it's one YAML entry + one line in `UpdateStates()`.
2. **Enum name `InverterState` instead of `ACState`.** The generator makes `eACState` from `ACState`, and that
   clashes with `eACState` in `AC.h` (compile error). Same values, only the YAML name differs:
   `{0: Off, 1: On, 2: OnKeepSetting, 3: KeepOn, 4: AutoOff}`. (`Off`/`On` are quoted in the YAML, otherwise
   YAML reads them as false/true.)
3. **IOBOARD_D has only 2 lights** in the code (the brief warned about 4 vs 11): OUT5 washbasin (umyvadlo) and
   OUT6 kitchen counter (kuch linka). Every other output is unused or the sewage pump (OUT1).
4. **ELECON exterior light is published as % (0 / 100), not bool.** The code only switches 0 or 100 (OUT1 and OUT4
   together). I kept % so that it appears with the lights (timeline, "on now", on-time per day), the same as the
   ELECON_D exterior light. Easy to change to bool.
5. **ELECON had no `AC_GetState()`.** I added a read-only getter to `ASW_ELECON/Core/Src/AC.c` (same as the one
   ELECON_D already has). No other AC change.
6. **RECU has no `UhaCommon/` copy any more.** It links `UHA_COMMON` like the other projects (`.project`), so the
   brief's sync step was not needed.
7. **`power_w: 12`** is a placeholder on the four lights that have it (washbasin, kitchen counter, Kolarna, Sklep).
   The exterior lights have no `power_w`. Tell me the real values if you want them right.
8. **Write timing.** Lights are written every 50 ms (`LC_Update_50ms()`, as on IOBOARD_U). Relays, inverter
   states, RECU states and the sewage pump are written once per second, because they are only switched in 1 s
   tasks. ELECON_D's `LEMON_Update_1s()` (EVSE / heating relay) runs after `APP_Update_1s()`, so these two states
   can lag the relay by up to 1 s.

## Per node

All new variables: subgroup `States.<Node>`, `send: on_change` (inherited), `period_ms: 20000`, `agg: event`,
no `legacy`. Flash / RAM delta is measured against v002.00 (the `.map` files); it includes the rows of all nodes
(VarMeta table + `Vars2` + VARS2 state, ~16 B RAM per row), because every node carries the full table.

### IOBOARD_D v002.01 — CRC `0x71C5BF52`, flash +1368 B, RAM +528 B

| id | name | type | source in the code |
|----|------|------|--------------------|
| 420 | ST_LIGHT_WASHBASIN | uint32 % | OUT5 umyvadlo, `PWROUT_GetPct()` in `LC_Update_50ms()` |
| 421 | ST_LIGHT_KITCHEN_COUNTER | uint32 % | OUT6 kuch linka, same |
| 422 | ST_SEWAGE_PUMP | bool | `SEWAGE_PUMP_OUTPUT` (OUT1) > 0, end of the pump schedule in `APP_Update_1s()` |

Files: `lightctrl.h/.c` (state pointer per light, same as IOBOARD_U), `APP.c` (`ConfigOutput()` gets the field,
group valid after init, pump line).

### ELECON_D v002.01 — CRC `0x5FC401C3`, flash +1480 B, RAM +536 B

| id | name | type | source in the code |
|----|------|------|--------------------|
| 490 | ST_AC_AXPERT | enum InverterState | REL1, `AC_GetState(eDCAC_Axpert)` |
| 491 | ST_AC_VICTRON | enum InverterState | REL3, `AC_GetState(eDCAC_Victron_2kW)` |
| 492 | ST_AKUPACK_HEATER | bool | REL2 output register (`AkupackHeatingControl()` sets it) |
| 493 | ST_AKUPACK_SET_DAY_C | float °C | `mAkupackSetDay_C` = 10.0 |
| 494 | ST_AKUPACK_SET_NIGHT_C | float °C | `mAkupackSetNight_C` = 7.0 |
| 495 | ST_EVSE_RELAY | bool | OUT1 `OUT_EVSE_RELAY` > 0 (LEMON sets 0 / 1 %) |
| 496 | ST_WS_HEATING_RELAY | bool | OUT4 `OUT_HEATING_RELAY` > 0 (LEMON) — see point 1 |
| 497 | ST_LIGHT_EXTERIOR_WS | uint32 % | OUT2 PWM (`ExtLightControl()`) |
| 498 | ST_LIGHT_KOLARNA | uint32 % | DO1 relay light, `ActualState` → 0 / 100, `LC_Update_50ms()` |
| 499 | ST_LIGHT_SKLEP | uint32 % | DO2 relay light, same |

Akupack setpoints: the two local constants in `AkupackHeatingControl()` are now static module variables with the
same values; nothing else changed in the heater logic. Relays / AC / setpoints are written in the new
`UpdateStates()` at the end of `APP_Update_1s()`. Lights: `lightctrl` gets a state pointer per light (relay
lights publish 0 / 100, PWM lights the duty).

### ELECON v002.01 — CRC `0x03950A26`, flash +1336 B, RAM +536 B

| id | name | type | source in the code |
|----|------|------|--------------------|
| 480 | ST_AC_300W | enum InverterState | REL1, `AC_GetState(eDCAC_300W)` (new getter, point 5) |
| 481 | ST_AC_3KW | enum InverterState | REL2, `AC_GetState(eDCAC_3kW)` |
| 482 | ST_AC_5KW | enum InverterState | REL3, `AC_GetState(eDCAC_5kW)` |
| 483 | ST_LIGHT_EXTERIOR_HOUSE | uint32 % | `PWROUT_GetPct(OUT1)`, OUT1 + OUT4 switched together (point 4) |

Written in `UpdateStates()` in `APP_Update_1s()`; `AC_Update_1s()` runs just before it, so it is current.
The ELECON tunnel already forwards unknown ids (frame hook), so the RPi gets the other nodes' states even before
ELECON is flashed.

### RECU v002.01 — CRC `0x0DCEC5AA`, flash +1396 B, RAM +536 B

| id | name | type | source in the code |
|----|------|------|--------------------|
| 510 | ST_RECU_REMOTE_MODE | enum RecuRemoteMode | `mRemoteMode` (`eRecuRemoteReqMode` in COM.h, same order) |
| 511 | ST_RECU_LIMIT | enum RecuLimit | highest-priority limitation of the last `RECON_Update_1s()` |

`RecuRemoteMode`: 0 Auto, 1 SlightOverpressure, 2 MaxOverpressure, 3 SlightUnderpressure, 4 MaxUnderpressure.

`RecuLimit` (higher number = higher priority; the mapping is also in the YAML comment):

| value | name | RECON branch where it is set |
|-------|------|------------------------------|
| 0 | None | nothing below was taken |
| 1 | Eco | SOC < 85 % and the fan limit is lowered to `FAN_MAX_DAY_ECO` |
| 2 | AntiDry | `mAntiDryOn` = 1 (waste RH ≤ 50 %, off above 52 %), fan limit `FAN_ANTI_DRY` |
| 3 | AntifreezeReduced | waste cold 4..8 °C: fresh fan reduced (non-summer branch) |
| 4 | AntifreezeStop | waste cold < 4 °C: fresh fan off (always applies) |
| 5 | BatteryOff | SOC < 30 %: fans 0 / 10 %, early return |
| 6 | InvalidInputs | a needed input invalid: fans 0 / 10 %, early return (code has this branch too) |

`RECON` is not restructured: one `Limit(x)` call in each of these branches, and `PublishStates()` at each of the
three exits. Note: in summer cooling mode the Eco / AntiDry limits are computed (so they show) but the summer
branch does not use the fan limit — that is the existing behaviour, the state only tells which branch ran.
Known existing behaviour, unchanged: during the early returns (invalid inputs, battery off) the remote request
timer does not count down.

## Shared parts

- **YAML** (`UHA_COMMON/VarDefs/uha_vars.yaml`): 19 new variables (291 values, max id 511), enums
  `InverterState`, `RecuRemoteMode`, `RecuLimit`. Validator 0 errors / 0 warnings, generated files regenerated.
- **Tests:** VarDefs tests all pass (validator 69/69, compile check, vars2, com, mcan, tunnel); UhaGatePy 72 tests
  pass on the PC and on the Pi. `test_vars_def.py` now ignores all enums that are not in the old flat YAML (one line).
- **Builds:** all four nodes build with the final YAML; no new compiler warnings in the changed code (the warnings
  in the logs are the existing ones). Every `Deployment/v002.00/` was left as it was. (ELECON's v002.00 `.map` had been
  overwritten by the 2a check build — `.map` is git-ignored, so `git checkout` did not restore it. I put back the
  copy I kept from before; `.bin` / `.hex` were never affected.)
- **IOBOARD_U / TECHM** are not rebuilt: their tables simply don't know the new ids, which they ignore.

## RPi (deployed)

- UhaGatePy: no code change, only the new YAML (`deploy.sh`), restarted 22:18. Journal clean: 291 variables,
  `33 event variables`, the IOBOARD_U events are already being written (27 rows in `events` at 22:17).
  `uha2.json` lists all 33 `ST_*` entries (the new ones invalid until the nodes are flashed).
- Grafana **UHA States** (`make_dashboards.py`, generated from the YAML): now one row per node —
  lights timeline, "Relays" timeline (bool), one timeline per enum (InverterState, BlindCmd, RecuRemoteMode,
  RecuLimit), "Setpoints ELECON_D" (stepped line, °C), and at the end "Lights on now" + "Light on-time per day" over
  all lights. Deployed; I could not open it in the browser (needs your login), please have a look.
- The UHARPI branch is not merged yet (main still has 0.4.0; the deployed code is identical, only the dashboard JSON
  and the test differ).

## Commits (branch `feature/vars2-states-2b`)

UHA_COMMON (YAML, generated files, this report), ASW_IOBOARD_D, ASW_ELECON_D (incl. CLAUDE.md OUT4), ASW_ELECON,
ASW_RECU_NUCLEO (each: code, `APP_INFO.h` 201, CHANGELOG, `Deployment/v002.01`), UHARPI (dashboard + test).
