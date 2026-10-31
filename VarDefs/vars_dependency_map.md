# UHA VARS — dependency map (state 2026-10-31)

Generated from live code of all ASW projects (`COM_AddStreamedVariable`, `VAR_SetVariable`, `TEMP_AssignSensor`, `ELM_AddMeter`, `VAR_GetVariable*`) and ELECON `SCOM.c` (RPi forward list). Dead code excluded: `ASW_ELECON/ELECON.c`, `BMS1.c`, `BMS2.c` (not called), `ASW_ELECON_D/ELECON.c` (not called). EVSE is not on the bus.

## Cross-node reads (control dependencies)

| Reader | Reads | From |
|---|---|---|
| ELECON | `VAR_LLOAD2_CONS_WH` | ELECON_D |
| ELECON | `VAR_LLOAD2_POWER_W` | ELECON_D |
| ELECON | `VAR_LPV2_POWER_W` | ELECON_D |
| ELECON | `VAR_LPV2_PROD_WH` | ELECON_D |
| ELECON | `VAR_POW_KITCHEN_W` | IOBOARD_D |
| ELECON | `VAR_STRG2_BALANCED_TODAY` | ELECON_D |
| ELECON | `VAR_STRG2_CAPACITY_AH` | ELECON_D |
| ELECON | `VAR_STRG2_CURRENT_A10` | ELECON_D |
| ELECON | `VAR_STRG2_ENERGY_WH` | ELECON_D |
| ELECON | `VAR_STRG2_OPT_CHARGING_A` | ELECON_D |
| ELECON | `VAR_STRG2_POWER_W` | ELECON_D |
| ELECON | `VAR_STRG2_VOLTAGE_V10` | ELECON_D |
| ELECON_D | `VAR_BAT_SOC` | ELECON |
| ELECON_D | `VAR_MPPT_SOLAR_VOLTAGE_V100` | ELECON |
| TECHM | `VAR_BAT_CURRENT_A10` | ELECON |
| TECHM | `VAR_BAT_SOC` | ELECON |
| TECHM | `VAR_BAT_VOLTAGE_V10` | ELECON |
| TECHM | `VAR_CHARGING_A10` | ELECON |
| TECHM | `VAR_MPPT_SOLAR_VOLTAGE_V100` | ELECON |
| TECHM | `VAR_TEMP_DOWNSTAIRS` | IOBOARD_D |
| TECHM | `VAR_TEMP_KIDROOM` | ELECON |
| TECHM | `VAR_TEMP_OUTSIDE` | ELECON |
| RECU | `VAR_BAT_SOC` | ELECON |
| RECU | `VAR_TEMP_DOWNSTAIRS` | IOBOARD_D |
| RECU | `VAR_TEMP_KIDROOM` | ELECON |
| RECU | `VAR_TEMP_OUTSIDE` | ELECON |

Summary:

- **ELECON ↔ ELECON_D: mutual dependency.** ELECON (GEMON) reads `STRG2_*`, `LLOAD2_*`, `LPV2_*` from ELECON_D; ELECON_D (LEMON load control) reads `BAT_SOC` and `MPPT_SOLAR_VOLTAGE_V100` from ELECON.
- **TECHM** reads battery data from ELECON (`BAT_SOC`, `BAT_CURRENT_A10`, `BAT_VOLTAGE_V10`, `CHARGING_A10`, `MPPT_SOLAR_VOLTAGE_V100`) and temperatures from ELECON (`TEMP_OUTSIDE`, `TEMP_KIDROOM`) and IOBOARD_D (`TEMP_DOWNSTAIRS`).
- **RECU** reads `BAT_SOC` (ELECON) and temperatures from ELECON and IOBOARD_D.
- **ELECON** also reads `POW_KITCHEN_W` from IOBOARD_D.
- **IOBOARD_D, IOBOARD_U**: no reads from other nodes.
- Nobody reads data produced by TECHM, RECU or IOBOARD_U (only RPi monitoring).

## Producers (variables on CAN per node)

| Node | Streamed vars | Frames/s (sum of 1000/period) |
|---|---|---|
| ELECON | 10 | 7.3 |
| ELECON_D | 76 | 31.9 |
| TECHM | 31 | 11.0 |
| IOBOARD_D | 12 | 10.7 |
| IOBOARD_U | 5 | 3.5 |
| RECU | 11 | 3.7 |

## ELECON-local variables forwarded only to RPi (not on CAN)

118 variables (BMS2/BMS3 incl. cells, STRG1/LLOAD1/LPV1, MPPT, global energy totals, AC meters …), ~44 frames/s at their RPi scan periods. Marked `on_can: false` in the YAML.

## Defined but not published by anyone (14)

`NETWORK_STATUS`, `LPV1_S1/S2_*`, `LPV2_S1/S2_*`, `LVDC_*`, `AXPERT_DISCHARGING_W`, `CURR_RECU_A` — marked `producer: NONE` in the YAML.

## Implicit ID arithmetic

`ELM.c` writes power to `energy_id + 10` (`VAR_SetVariable(mMeters[i].VarId + 10, …)`), e.g. `CONS_KITCHEN_A_WH` (68) → `POW_KITCHEN_A_W` (78). The new definition must keep this relation or ELM must get the power ID explicitly.
