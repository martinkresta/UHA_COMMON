# KI-004: ELECON `BMS.c`: `Active` never cleared on timeout, LEMON uses stale BMS data

| | |
|---|---|
| Status | open |
| Severity | medium |
| Area | `UHA_COMMON/Src/BMS.c` (used by ASW_ELECON), `ASW_ELECON/Core/Src/LEMON.c` |
| First seen | 2026-11-15 (code review during [KI-002](KI-002_bms2_invalid_13min.md)) |
| Occurrences | every BMS timeout, e.g. KI-002 (13 min), KI-003 (~40 s every 54 min) |

## Symptom

None visible directly. While a BMS is invalid (VALID = 0 in the tunnel), ELECON's own logic keeps using the
last received BMS values as if they were current.

## Analysis

- `BMS_Update()` sets `bms->Active = 1` on the first valid frame and **never sets it back to 0**. The timeout
  only affects the published VARS2 valid flag: `Publish()` uses `fresh = Active && TimeoutCounter < BMS_TIMEOUT_S`.
- The helper functions check only `Active`:
  - `BMS_IsChargingEnabled()` and `BMS_IsDischargingEnabled()` keep returning the last status bits.
  - `BMS_GetMinCellVoltage()` keeps returning the last minimum.
  - The fallbacks for "no BMS" (return 1 / 3600) are never used after the first frame.
- `ASW_ELECON/Core/Src/LEMON.c` decides on that:
  - `CheckFullyCharged()`: "fully charged" (resets `Avilable_mAs` to 100 %) from `!BMS_IsChargingEnabled` + SOC >= 99.
  - `CheckFullyCharged()`: "balanced today" (also resets to 100 %) from `LiveData.MinCellVoltage_mV` of an `Active`
    BMS.
  - `SocReinit()`: initial SOC from `LiveData.SOC` of an `Active` BMS.
- So during a BMS dropout the SOC estimation / full-charge detection may act on data up to many minutes old.
  No direct charge/discharge switching was found in ELECON using these helpers (the BMS hardware protects the
  pack itself), hence severity medium, not high.
- Reference: ELECON_D's `JKBMS.c` already does it right: it clears `Active` on timeout and the helpers fail
  closed (charging/discharging disabled, min cell 3600 mV).

## Proposed fix

In `UHA_COMMON/Src/BMS.c`, either:
- clear `Active` when `TimeoutCounter` reaches `BMS_TIMEOUT_S`, or
- add `BMS_IsFresh()` (= the `fresh` expression from `Publish()`) and use it in the helpers and in `LEMON.c`
  instead of `Active`.

Decide the fallback per use: for `CheckFullyCharged()` "no data" must mean "do not declare full / balanced".
Check what else links `UHA_COMMON/Src/BMS.c` before changing it (grep `BMS_Init`), and bump `ASW_VERSION` of each
affected node.
