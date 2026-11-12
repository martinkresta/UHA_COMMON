# Task: VARS2 step 6b — nodes stop sending the old variable format

After step 6a all nodes read remote variables from VARS2. Now the old format stops: CAN `0x110+node` on every
node and ELECON's `0x221` scan list to the RPi. The old receive path stays in the code (later cleanup).

**How this task is run:** Part A once, then Part B for one node at a time. Martin says which part/node to do.
Do only that.

## Context

Read first: `UHA_FW/CLAUDE.md`, `UHA_COMMON/CLAUDE.md`, the target project's `CLAUDE.md`,
`UHA_COMMON/VarDefs/VARS2_spec.md`, `UHA_COMMON/VarDefs/README.md` (YAML, validator, generator),
`UHA_COMMON/Src/COM.c`, `UHA_COMMON/Src/VARS2.c`.

**Prerequisites:**
- `main` of every repo holds the deployed firmware (6a read layer, CAN TX supervision, GEMON fix).
- UhaGatePy task `UHARPI/tasks/uhagatepy_vars2_only.md` is deployed: once a variable has come over VARS2, the
  gateway never falls back to the old value. So the nodes below can switch in any order.

Facts checked in the code:
- Old CAN TX is one line: `COM.c` `SendVariable()` → `MCAN_SendFrame(CMD_VAR_VALUE + mNodeId, ...)`, followed
  by `VARS2_SendLegacy(id)`. All streamed variables of all six nodes are in the YAML.
- ELECON `Core/Src/SCOM.c` `SCOM_Update_10ms()` sends `0x221`: `VAR_NETWORK_STATUS` every 5 s
  (`SCOM.c:85-92`) and the 271-entry scan list. Everything in the scan list that is in the YAML already comes
  through the tunnel. **`VAR_NETWORK_STATUS` (id 1) is not in the YAML** but is a `uha.json` key, so it has to
  be added. The other 19 entries not in the YAML (`AXPERT_DISCHARGING_W`, `LPV1/2_S1/S2_*`, `LVDC_*`,
  `CURR_RECU_A`) are set by no firmware; they are dropped.
- `0x620` events, RTC (`0x051`), SysInfo, NMT, bootloader, the tunnel: unchanged.
- IOBOARD_D, IOBOARD_U and TECHM have their own `SCOM.c` with `VAR_NETWORK_STATUS`; not part of this task.

## Rules

- Live network: agents never flash. Martin does it.
- **Build only the node being changed.** The post-build step writes `Deployment/vXXX.YY/` by `ASW_VERSION`.
- Never overwrite or revert Martin's uncommitted files. `git status` first; ask about anything unexpected.
- Small logical commits on the feature branch. Do not merge.

## Part A — shared (`UHA_COMMON`)

Branch `feature/vars2_stop_legacy_tx` from `main`. No node build, no version bump.

1. YAML: new group `system`, producer ELECON, with
   `id: 1, name: NETWORK_STATUS, legacy: VAR_NETWORK_STATUS, type: uint32, period_ms: 5000, on_can: false,
   receivers: [RPI]`. Run the validator, regenerate `generated/` and the FW code, check `VarMeta` has the row.
2. `COM.c/.h`:
   ```c
   static uint8_t mLegacyTxOff;
   void COM_DisableLegacyTx(void) { mLegacyTxOff = 1; }
   ```
   In `SendVariable()` skip only the `MCAN_SendFrame(CMD_VAR_VALUE + ...)` when `mLegacyTxOff`;
   `VARS2_SendLegacy(id)` always runs. Keep the same copy in `ASW_RECU_NUCLEO/UhaCommon` identical (done in its
   Part B).
3. PC unit test: disabled = today's frames; enabled = only `0x180+node`; stream timing unchanged.
4. Independent reviewer subagent. Report to Martin, and remind him to redeploy UhaGatePy with the new YAML
   (`deploy.sh` copies it) **before** flashing ELECON.

## Part B — one node

Branch `feature/vars2_stop_legacy_tx` from `main` in the node's repo. `UHA_COMMON` must contain merged Part A.

1. Call `COM_DisableLegacyTx();` in `APP.c` right after `VAR_EnableVars2Read();` (IOBOARDs: after
   `COM_Init(THIS_NODE);`).
2. Bump `ASW_VERSION` by 1 in this project only (own commit), check the new `Deployment/` folder does not exist,
   build this node only, add the `Deployment/CHANGELOG.md` entry, commit artifacts and changelog.
3. Independent reviewer subagent.
4. Report: version + CRC, flash/RAM before → after, and what Martin checks after flashing: no `0x110+node`
   from this node in the PC tool, bus load lower, UhaGatePy coverage tool still all `vars2`, no frozen-node
   warnings.

| Order | Node (repo) | Extra |
|---|---|---|
| 1 | IOBOARD_U (`ASW_IOBOARD_U`) | — |
| 2 | IOBOARD_D (`ASW_IOBOARD_D`) | — |
| 3 | RECU (`ASW_RECU_NUCLEO`) | First sync `UhaCommon` (`COM`, `uha_vars_gen`) with `UHA_COMMON`, separate commit |
| 4 | ELECON_D (`ASW_ELECON_D`) | — |
| 5 | TECHM (`ASW_TECHM`) | — |
| 6 | ELECON (`ASW_ELECON`) | See below |

**ELECON:** in `SCOM_Update_10ms()` stop the `0x221` traffic: the scan list and the `VAR_NETWORK_STATUS`
`SendVariable()`. Keep `VAR_SetVariable(VAR_NETWORK_STATUS, COM_GetNetworkStatus(), 1)` every 5 s: it now
reaches the RPi through `VARS2_UpdateLocal_10ms()` (ELECON, `on_can: false`). Use one flag
(`mLegacyScanOff`, set in `SCOM_Init()`), keep the scan-list code for fallback. Check in the unit/tunnel test
that `NETWORK_STATUS` leaves on `0x184` every 5 s and no `0x221` leaves. After flashing Martin checks
`VAR_NETWORK_STATUS` in `uha.json` and that UhaGatePy counts no old packets. Note in the report: from now on
UhaGate6 (`rollback_to_v6.sh`) has no live data.
