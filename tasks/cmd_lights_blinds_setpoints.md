# Commands: remaining lights, blinds, setpoints (step 3b)

Spec: `UHA_COMMON/VarDefs/CMD_spec.md`. Pattern to copy: step 3a in `ASW_IOBOARD_U/Core/Src/APP.c`
(`APP_OnCommand()` with `CmdLight(OUTn, value, CMD_..._MIN, CMD_..._MAX)`), YAML `commands:` section.
Runs together with the RPi brief `UHARPI/tasks/control_ui_types.md` (UI support for enum/float commands).

Lightweight mode as in 3a: implement → build → short report; **Martin flashes**; `ASW_VERSION` bump +
`Deployment/CHANGELOG.md` per changed node; branch `feature/cmd-3b`. Work **node by node** in the order below.
**TECHM rule (as in 2c):** no change of any control decision; the report lists every changed line in TEMPCON,
BOILER_AIR, HEATING, ELHEATER, DO and why it cannot change behaviour. Martin reads it before flashing.

**New optional YAML key `category:`** for commands (UI grouping only, not used by FW; add to the validator as a
known key with allowed values `light`, `blind`, `heating`, `inverter`, `ventilation`). Set it on every command,
**including the 3a IOBOARD_U lights** (`light`). Lights → `light`, screens → `blind`, TECHM and akupack setpoints →
`heating`. Commit the YAML early — the UI agent groups by it.

Clamping: always in a small setter of the owning module (or a static helper next to it), using the generated
`CMD_<NAME>_MIN/_MAX` — never in `CMD.c`. A setter is the only place that writes the value from outside the module.

## 1. IOBOARD_D (ids 100–199)

- Lights published in `States.IoboardD`: `ST_LIGHT_WASHBASIN`, `ST_LIGHT_KITCHEN_COUNTER` → commands
  `CMD_LIGHT_WASHBASIN`, `CMD_LIGHT_KITCHEN_COUNTER`, `uint32` 0..100, via `LC_RemoteControl()` like 3a.
- Sewage pump: **no command** (runs on its schedule).

## 2. IOBOARD_U — blinds (ids 2xx, continue after the lights)

- `CMD_SCREEN_EAST`, `CMD_SCREEN_WEST`: `type: enum`, `enum: BlindCmd`, state `ST_SCREEN_EAST/WEST`.
- Handler calls `WSCRC_ManualControl(screenId, dir)` (`WSCRC.c`). Today it takes only up/down (`dir`), started
  from `escs_idle`. Study what a call does while the screen is moving (stop? ignored?) and map:
  Up → up, Down → down, Stop → only if the module already has a way to stop; otherwise ignore Stop and say so in
  the report (do not add new motor logic). `None` → ignore.
- Note: `WSCRC_ManualControl()` uses `target` uninitialised for an unknown `screenId` — the handler must only pass
  `SCRID_EAST` / `SCRID_WEST` (and fix that line with an early return, nothing else).

## 3. ELECON_D (ids 700–799)

- Lights `ST_LIGHT_KOLARNA`, `ST_LIGHT_SKLEP` → `CMD_LIGHT_KOLARNA`, `CMD_LIGHT_SKLEP` (0..100, `LC_RemoteControl()`;
  keep their auto-off timers working as for a button press — check and report).
- Akupack heater setpoints `mAkupackSetDay_C` / `mAkupackSetNight_C` (`APP.c`) → `CMD_AKUPACK_SET_DAY`,
  `CMD_AKUPACK_SET_NIGHT`, `float` °C, **range 0..15**, state `ST_AKUPACK_SET_DAY_C/NIGHT_C`. Setter with clamp.
- Not now: exterior light (automatic dusk/dawn), EVSE relay (LEMON owns it), inverters (step 3c).

## 4. TECHM (ids 300–399)

| Command | Variable | Range | State |
|---|---|---|---|
| `CMD_UP_REQ_DAY` | `mUpReqTempDay` (TEMPCON) | 5..25 °C | `ST_UP_REQ_DAY_C` |
| `CMD_UP_REQ_NIGHT` | `mUpReqTempNight` | 5..25 °C | `ST_UP_REQ_NIGHT_C` |
| `CMD_DOWN_REQ_DAY` | `mDownReqTempDay` | 5..25 °C | `ST_DOWN_REQ_DAY_C` |
| `CMD_DOWN_REQ_NIGHT` | `mDownReqTempNight` | 5..25 °C | `ST_DOWN_REQ_NIGHT_C` |
| `CMD_EXHAUST_SET` | `mBacControlTemp_C` (BOILER_AIR) | 150..250 °C | `ST_EXHAUST_SET_C` |

- All `float`, `step` 0.5 for rooms / 5 for exhaust as a UI hint (new optional key `step:` in `commands:`;
  add it to the validator, not used by FW).
- Setters `TC_SetUpReqTempDay()` … and `BAC_SetControlTemp()` with clamp; make the variables `static` if nothing
  else writes them (check, report).
- A changed room setpoint takes effect at the next TEMPCON cycle — no immediate valve/pump action from the setter.
- Values stay in RAM (default after reset, as agreed; re-send from RPi is a later topic).

## Report and stop

Per node: version, commands added (id, name, handler), RAM/flash delta, the TECHM line list, how the blinds Stop
and the ELECON_D auto-off behave. Stop — Martin flashes and tests via `ctrl.uha.mkresta.cz`.
