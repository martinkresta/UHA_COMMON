# VARS2 states: send on change + event logging (step 2a, pilot IOBOARD_U)

Repos: **UHA_FW** (UHA_COMMON VARS2 + YAML + tools, ASW_IOBOARD_U) and **UHARPI** (UhaGatePy, Grafana).
Follow-ups: `UHA_COMMON/tasks/states_nodes.md` (2b: IOBOARD_D, ELECON_D, ELECON, RECU) and
`ASW_TECHM/tasks/techm_states_setpoints.md` (2c). This brief builds the common mechanism; 2b/2c only add states.

## Goal

Discrete states (lights, pumps, relays, inverter states, setpoints, modes) become normal VARS2 variables in a new
YAML group `States`. They are sent **when they change** (plus a slow refresh) and the RPi logs them as **events**
(one row per change), so Grafana can show timelines and "how long was the light on".

Agreed design (do not redesign):
- No new protocol, no separate `STATES.c`: VARS2 gets one new TX mode. Frame, COB-ID (`0x180+node`), RX, timeout,
  ELECON tunnel and UhaGatePy parsing stay as they are.
- Later (step 3) commands will use a separate command channel; states are the feedback of those commands.
- One SQLite DB, new table `events`.

## Working mode (lightweight, also for FW)

- Brief → implement → build → short report. No new unit tests, no separate reviewer agent. Existing tests
  (`VarDefs/tests/*`, UhaGatePy tests) must keep passing; extend an existing test only where it is a one-liner.
- **Martin flashes every node himself.** Agents never flash. Agent delivers the `Deployment/` build.
- Every node whose FW changes: bump `ASW_VERSION` before the first build, add a `Deployment/CHANGELOG.md` entry,
  build only that node. Restore `Deployment/` after check builds of other nodes.
- RPi part: deploy directly, run, debug from the journal (rules as in `UHARPI/tasks/vars2_db_logging_grafana.md`).
- Git: feature branch `feature/vars2-states` in UHA_FW and UHARPI. Commit, Martin pushes.

## Part 1 — YAML and tools (UHA_COMMON/VarDefs)

New keys (document them in the YAML header and in `VARS2_spec.md`):

```yaml
send: on_change   # TX when value or valid changes (min 1 s apart) + every period_ms as refresh; default: periodic
agg: event        # UhaGatePy: log one row per change (no log_s); existing: avg | delta with log_s
```

- `send` allowed on a variable and on a group/subgroup (inherited, like `producer`).
- For `on_change` variables `period_ms` is the refresh period: use **20000**. Reason: `TimeoutMs` in `sVarMeta`
  is `uint16_t` and the validator checks `auto` timeout (3 × period) fits u16 — 3 × 20000 = 60000 fits; 60000 would not.
- Validator: accept the keys; `agg: event` must not have `log_s`; `agg: avg|delta` still need `log_s`.
- Generator: new flag `VMF_ON_CHANGE 0x02u` in `VarMeta[].Flags` (`VARS2_types.h`).
- New top-level group (no producer on the container, one producer per subgroup, like `Temperatures`):

```yaml
  States:   # event-based states, send: on_change (VARS2_spec.md)
    send: on_change
    subgroups:
      IoboardU:
        producer: IOBOARD_U
        vars: [...]
```

- **Id ranges** (keep free space per node): IOBOARD_U 400–419, IOBOARD_D 420–439, TECHM 440–479, ELECON 480–489,
  ELECON_D 490–509, RECU 510–519.
- One variable per light/output with a readable name (no arrays), e.g. `ST_LIGHT_LOFT_WEST`, field
  `LoftWest_pct`. Light names: from the code (`LC_AddLight` calls / comments in `APP.c`); if a name cannot be
  found, ask Martin — do not invent.
- Lights: `type: uint32`, `unit: "%"` 0–100, plus `power_w: 12` (new documentation key, not used yet — add it to
  the validator's known keys). All states get `agg: event`. No `legacy` key (they are not in `uha.json`).
- New enums in `enums:` as needed (this step: `BlindCmd`).

## Part 2 — VARS2.c (UHA_COMMON/Src)

Change only the TX decision; everything else stays.

- `IsDue()`: an own variable with `VMF_ON_CHANGE` is also due when its 4-byte value or its valid flag differ from
  what was **last sent**, and at least `VARS2_ON_CHANGE_MIN_MS` (1000, a `#define`) passed since the last send.
  The normal period check stays (= refresh). Applies to both the CAN path and the tunnel path
  (`VARS2_UpdateLocal_10ms`).
- Remember the last sent value + valid when a frame is built for an on-change variable. Choose the simplest
  storage that fits RAM (e.g. extend `sVars2State`, or a small array only for on-change rows indexed by the
  generator). **Report the RAM/flash delta** from the `.map` of IOBOARD_U and ELECON (L431, smallest RAM).
- One clearly commented function for the change check, so the behaviour is obvious when reading `VARS2.c`.
- Modules do not call anything new: they write `Vars2.States.<Node>.<Field>.value/.valid` like any produced group.
- Note: ELECON's frame hook already forwards every received frame, also unknown ids
  (`VARS2_ProcessRxFrame()` calls the hook independent of `MetaOf()`), so ELECON does **not** need a reflash for
  new ids of other nodes. Nodes with old FW ignore unknown ids. → nodes can be flashed one by one.
- `ASW_RECU_NUCLEO/UhaCommon/` is a copy: sync it in 2b, not now (RECU is not rebuilt in this step).

## Part 3 — Pilot node IOBOARD_U

States (subgroup `States.IoboardU`, producer IOBOARD_U):
- every light registered by `LC_AddLight` (the code has 8 calls; `CLAUDE.md` lists 12 outputs — **the code wins**):
  intensity actually applied 0–100 % (`sLightSource.ActualIntensity` / the PWM value really set — pick the one
  that is the real output state and say which in the report).
- 2 sunscreens (`SCRID_EAST`, `SCRID_WEST`): last command as enum `BlindCmd` {0: None, 1: Up, 2: Down, 3: Stop}
  (map from the `dir` values of `WSCRC_ManualControl()` and any other command source in `WSCRC.c`; only what
  exists). The real position is unknown (stateless control) — that is accepted.
- Write the values in an existing periodic APP/LC/WSCRC update (≤ 100 ms), set the group valid once after init.
  No change of light or sunscreen behaviour.
- Bump `ASW_VERSION`, CHANGELOG, build `Deployment/`. Martin flashes.

## Part 4 — UhaGatePy + Grafana (UHARPI)

- `vars_def.py`: `legacy` becomes optional (variable without `legacy` → no `uha.json` key, only `uha2.json`);
  read `send` (ignored by the gateway) and `agg: event`. States appear in `uha2.json` automatically — check.
- `db_log.py`: new table
  `CREATE TABLE IF NOT EXISTS events (var_id INTEGER, ts INTEGER, value REAL, PRIMARY KEY (var_id, ts)) WITHOUT ROWID;`
  - For `agg: event` variables store a row when the value differs from the last stored one, or valid changed
    (invalid → `value NULL`). Refresh frames with the same value are dropped.
  - After a gateway start the first received value of each event variable is stored (state known again).
  - `ts` = receive time, UTC seconds (if two changes fall into one second, keep the last).
  - Kept forever (small: thousands of rows per day). Same writer thread, same batching and size guard.
  - Journal summary: add the events row count.
- Grafana (`UHA/Grafana/make_dashboards.py` + provisioning): new dashboard **States** with "State timeline"
  panels — lights per node, sunscreens. Value mappings from the YAML enums (generated, not typed by hand). One
  table/stat panel "lights on now" and one "light on-time per day" (hours, from `events`; computed in SQL,
  intervals between consecutive rows of one variable, value > 0, clipped to the local day).
- Bump `VERSION` (0.4.0), README section.

## Check after Martin flashed IOBOARD_U

- Journal: no errors, states fresh in `uha2.json`, `events` gets rows when a light is switched.
- CAN: on-change frames only on change + every 20 s (a quick look with the PC tool or the gateway's counters).
- Grafana States dashboard shows the switch.

## Stop points

1. Parts 1–4 done, IOBOARD_U build ready, UhaGatePy deployed → **report to Martin, wait for his flash and check.**
2. Then 2b (`states_nodes.md`) only after Martin's go.
