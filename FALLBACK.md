# Fallback baseline

Firmware and gateway versions that run together and work. If a later change causes severe problems, return
the whole system to one of these baselines: flash every node with the listed version (artifacts are committed
in each node's `Deployment/` folder) and deploy the listed gateway.

## 2026-11-14 — VARS2 only (after step 6b, before `vars2_direct_access`)

All nodes speak only VARS2 (`0x180+node`), the old format is removed. Modules still use the old `VAR_` API
internally.

| Node | Repo | Version | CRC | Artifacts commit |
|---|---|---|---|---|
| IOBOARD_D | `ASW_IOBOARD_D` | v001.03 | `0xA29ADE01` | `ef033cc` |
| IOBOARD_U | `ASW_IOBOARD_U` | v001.03 | `0x7142A388` | `44849fb` |
| TECHM | `ASW_TECHM` | v001.06 | `0x9C4988D8` | `0c09d63` |
| ELECON | `ASW_ELECON` | v001.16 | `0x40B99687` | `8dfb1e5` |
| RECU | `ASW_RECU_NUCLEO` | v001.04 | `0xF8C3B49E` | `63e9e55` |
| ELECON_D | `ASW_ELECON_D` | v001.20 | `0x913BB176` | `56fe772` |

| Component | Version | Commit |
|---|---|---|
| Shared code + YAML (`UHA_COMMON`) | — | `08221a3` |
| RPi gateway UhaGatePy (`UHARPI`) | 0.2.0, with the YAML from `UHA_COMMON` `08221a3` | `50d597a` |
