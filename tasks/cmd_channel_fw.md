# Command channel — firmware part (step 3a)

Spec: **`UHA_COMMON/VarDefs/CMD_spec.md`** (read it first; it wins if this brief is unclear).
Runs in parallel with the RPi brief `UHARPI/tasks/control_web_api.md` (other agent). This agent owns the YAML
`commands:` section and the generator.

Repos: UHA_FW (UHA_COMMON, ASW_ELECON, ASW_IOBOARD_U). Branch `feature/cmd-channel`.

## Working mode

Lightweight as in step 2 (brief → implement → build → short report; **Martin flashes**; `ASW_VERSION` bump +
`Deployment/CHANGELOG.md` for ELECON and IOBOARD_U). Two exceptions, because ELECON forwarding is the only place
where a bug could open the CAN bus to a compromised RPi:
1. **Independent review** of the ELECON forwarding code (`SCOM.c` changes) by a separate reviewer subagent that
   gets only the spec section 3 and the diff. Fix its findings or explain.
2. **One small host test** for the forwarding (reuse the existing stub/test setup in `UHA_COMMON/VarDefs/tests/`,
   e.g. like `test_tunnel`): CAN id is always `0x200 + (data[0] & 0x0F)` for any input byte; length ≠ 8 dropped;
   11th command within 1 s and 21st within 60 s dropped; target 4 goes to `CMD_ProcessFrame()` and not to CAN.

## Part 1 — YAML + tools (`UHA_COMMON/VarDefs`)

- `commands:` section and validator rules exactly as spec section 5; document in the YAML header and a short
  section in `VARS2_spec.md` pointing to `CMD_spec.md`.
- Pilot commands (spec section 6): one per IOBOARD_U light, ids 200+, names matching the `ST_LIGHT_*` states.
- Generator: `uha_cmds_gen.h/.c` with `eCmdId`, table, `CMD_<NAME>_MIN/_MAX`. Commit Part 1 early so the RPi
  agent can read the real YAML.
- Existing VARS2 generated files must stay byte-identical (the `commands:` section does not change them).
- `ASW_RECU_NUCLEO/UhaCommon/` copy: add the new files there too (RECU is not rebuilt now; keep identical).

## Part 2 — Common FW (`UHA_COMMON`)

- `CMD.c/.h` as spec section 4; `APP_OnCommand()` is declared in `CMD.h`, implemented per node. Nodes without
  commands get an empty weak default (or a trivial empty function) so every project still builds.
- `COM_GetRxMessage()`: consume `0x200 + own node` → `CMD_ProcessFrame()`.
- Check MCAN RX filters (`UHA_COMMON/Src/MCAN.c`, filter setup per node) let `0x200 + own node` in.
- Add `CMD.c` and the generated `.c` to the build of every ASW project that links UHA_COMMON (check how linked
  resources pick up new files in each `.cproject`; build every project once to prove it, then restore their
  `Deployment/` as usual — only ELECON and IOBOARD_U get new versions).

## Part 3 — ELECON forwarding (`ASW_ELECON/Core/Src/SCOM.c`)

- `#define CMD_RPI_COMMAND 0x52` in `SCOM.h`; new `case` in `ProcessMessage()` implementing spec section 3
  exactly (fixed base `0x200`, `& 0x0F`, DLC 8, 10/s + 20/min, local handling for target 4, LOG on drop).
- `APP_OnCommand()` for ELECON: empty `switch` for now (no ELECON commands in 3a).
- Check: the RPISERP RX length check really yields 8 data bytes for `len = 10` (see `RSP_GetMessage()` / `mRxLength`).

## Part 4 — IOBOARD_U handler

- `APP_OnCommand()` in `ASW_IOBOARD_U/Core/Src/APP.c`: map each `CMDID_LIGHT_*` to its `lightId`, clamp with
  `CMD_..._MIN/_MAX`, call `LC_RemoteControl(lightId, intensity)`. No other change of light behaviour; check that
  `LC_RemoteControl()` respects `LC_Lock()` the same way as button control does (report what it does).
- The VARS2 state (`States.IoboardU`) must change after a command (it already reflects the real output — verify).

## Report and stop

Versions, YAML commands added, files changed, RAM/flash delta (ELECON, IOBOARD_U), host test result, reviewer
findings and what was done. Stop — Martin flashes ELECON and IOBOARD_U (any order; without the RPi part nothing
sends commands).
