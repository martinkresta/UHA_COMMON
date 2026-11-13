# Task: VARS2 step 6b — remove the old variable format from the firmware

After step 6a all nodes read remote variables from VARS2, and UhaGatePy 0.2.0 reads only VARS2 (it ignores
`0x221`). Now the old format is **removed** from the firmware, not disabled: CAN `0x110+node` (TX and RX) on
every node and ELECON's `0x221` scan list to the RPi. Martin's direction: one protocol, simple code, no
fallback paths and no comparison tooling for the old format.

**How this task is run:** Part A once, then Part B for one node at a time. Martin says which part/node to do.
Do only that.

## Context

Read first: `UHA_FW/CLAUDE.md`, `UHA_COMMON/CLAUDE.md`, the target project's `CLAUDE.md`,
`UHA_COMMON/VarDefs/VARS2_spec.md`, `UHA_COMMON/VarDefs/README.md` (YAML, validator, generator),
`UHA_COMMON/Src/COM.c`, `UHA_COMMON/Src/VARS.c`, `UHA_COMMON/Src/VARS2.c`.

**Prerequisite:** `main` of every repo holds the deployed firmware (6a read layer, CAN TX supervision, GEMON
fix). Nobody reads the old format any more, so the nodes can change in any order.

Facts checked in the code:
- Old CAN TX: `COM.c` `SendVariable()` → `MCAN_SendFrame(CMD_VAR_VALUE + mNodeId, ...)`, then
  `VARS2_SendLegacy(id)`. Old RX: `CMD_VAR_VALUE` frames stored into `Vars[]` (find all places, incl. the
  application `case` in each `APP.c`). All streamed variables of all six nodes are in the YAML.
- `Vars[]` stays for now: it is the producer's own store (`VARS2_SendLegacy()` / `VAR_GetVariableLegacy()`)
  and the `VAR_GetVariable()` compatibility layer keeps the modules unchanged. Removing it is a later step.
- ELECON `Core/Src/SCOM.c` `SCOM_Update_10ms()` sends `0x221`: `VAR_NETWORK_STATUS` every 5 s
  (`SCOM.c:85-92`) and the 271-entry scan list. **`VAR_NETWORK_STATUS` (id 1) is not in the YAML**, so since
  UhaGatePy 0.2.0 the web shows it as 0. It has to be added. The 19 scan-list entries not in the YAML
  (`AXPERT_DISCHARGING_W`, `LPV1/2_S1/S2_*`, `LVDC_*`, `CURR_RECU_A`) are set by no firmware; they go.
- `0x620` events, RTC (`0x051`), SysInfo, NMT, bootloader, the tunnel, CAN TX supervision: unchanged.
- IOBOARD_D, IOBOARD_U and TECHM have their own `SCOM.c` with `VAR_NETWORK_STATUS`; not part of this task.

## Rules

- Live network: agents never flash. Martin does it.
- **Build only the node being changed.** The post-build step writes `Deployment/vXXX.YY/` by `ASW_VERSION`.
- Never overwrite or revert Martin's uncommitted files. `git status` first; ask about anything unexpected.
- Delete old-format code; do not keep it behind flags or `#if 0`. Git history is the fallback.
- Small logical commits on the feature branch. Do not merge.

## Part A — shared (`UHA_COMMON`)

Branch `feature/vars2_stop_legacy_tx` from `main`. No node build, no version bump.

1. YAML: new group `system`, producer ELECON, with
   `id: 1, name: NETWORK_STATUS, legacy: VAR_NETWORK_STATUS, type: uint32, period_ms: 5000, on_can: false,
   receivers: [RPI]`. Run the validator, regenerate `generated/` and the FW code, check `VarMeta` has the row.
2. `COM.c/.h`: delete the old TX frame in `SendVariable()` (only `VARS2_SendLegacy(id)` remains) and the old
   RX handling of `CMD_VAR_VALUE` in the shared code. Anything that only served the old format goes with it.
   List what you removed.
3. PC unit test: only `0x180+node` leaves, stream timing unchanged; a received `0x110+node` frame changes
   nothing.
4. Independent reviewer subagent. Report to Martin, and remind him to redeploy UhaGatePy so it gets the new
   YAML (`deploy.sh`), then `VAR_NETWORK_STATUS` appears once ELECON is flashed.

## Part B — one node

Branch `feature/vars2_stop_legacy_tx` from `main` in the node's repo. `UHA_COMMON` must contain merged Part A.

1. Delete the node's own old-format code: the `CMD_VAR_VALUE` `case` in `APP.c` and anything else in the
   project that only served the old format.
2. Bump `ASW_VERSION` by 1 in this project only (own commit), check the new `Deployment/` folder does not exist,
   build this node only, add the `Deployment/CHANGELOG.md` entry, commit artifacts and changelog.
3. Independent reviewer subagent.
4. Report: what was removed, version + CRC, flash/RAM before → after, and what Martin checks after flashing:
   no `0x110+node` from this node in the PC tool, bus load lower, UhaGatePy summary still all fresh, no
   frozen-node warnings, the node's control behaves as before.

| Order | Node (repo) | Extra |
|---|---|---|
| 1 | IOBOARD_U (`ASW_IOBOARD_U`) | — |
| 2 | IOBOARD_D (`ASW_IOBOARD_D`) | — |
| 3 | RECU (`ASW_RECU_NUCLEO`) | First sync `UhaCommon` (`COM`, `VARS2`, `uha_vars_gen`) with `UHA_COMMON`, separate commit |
| 4 | ELECON_D (`ASW_ELECON_D`) | — |
| 5 | TECHM (`ASW_TECHM`) | — |
| 6 | ELECON (`ASW_ELECON`) | See below |

**ELECON:** delete the `0x221` traffic from `SCOM.c`: the scan list (`InitPcScanList()`, `UpdateScanList()`,
the scan loop) and the `VAR_NETWORK_STATUS` `SendVariable()`. Keep
`VAR_SetVariable(VAR_NETWORK_STATUS, COM_GetNetworkStatus(), 1)` every 5 s: it now reaches the RPi through
`VARS2_UpdateLocal_10ms()` (ELECON, `on_can: false`). Check in the tunnel test that `NETWORK_STATUS` leaves on
`0x184` every 5 s and no `0x221` leaves. After flashing Martin checks `VAR_NETWORK_STATUS` on the web and that
UhaGatePy counts no old packets. From then on UhaGate6 has no live data.
