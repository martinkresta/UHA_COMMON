# VARS2 direct access — notes

Task: `UHA_COMMON/tasks/vars2_direct_access.md`. Branch `feature/vars2_direct_access`.

## Part A.1 — YAML review (PROPOSAL, waiting for Martin's approval)

Nothing in `uha_vars.yaml` is changed yet. Ids, types, `legacy`, `legacy_scale`, `period_ms`, `on_can`
stay as they are.

### Proposed YAML schema changes

| Key | Where | Meaning |
|---|---|---|
| group key | group | New CamelCase group name = field name in `Vars2` (`Vars2.StorageHome`) |
| `struct` | group, optional | Shared struct type. All groups with the same `struct` get one type `sVars<struct>`; validator checks identical field lists (field, type, count, unit). |
| `validity` | group, `group` (default) or `per_field` | `group`: one producer module sets all `.valid` with `VARS2_SetGroupValid()`. `per_field`: fields written by id (TEMP, ELM, SENSIRION), each with its own validity. |
| `field` | variable, required | C field name in LEMON style with unit suffix (`Soc_pct`). |
| `name` | variable | **Unchanged.** So `VID_*` ids and the names in `uha2.json` stay the same. |
| `producer` | group only | A per-variable `producer` override is no longer allowed (validator: one producer per group). |

Unit suffixes: `_pct` (%), `_V`, `_A`, `_W`, `_Wh`, `_Ah`, `_C` (°C), `_ppm`, `_Pa`, `_lpm` (l/min), `_l`,
`_mps` (m/s). Bool and enum fields have no suffix.

### Groups

| New group | `struct` | Producer node / module | Validity source today | Old group |
|---|---|---|---|---|
| `System` | | ELECON `SCOM.c` | always 1 | system |
| `Battery` | | ELECON `GEMON.c` | `mBatValid` | global_energy |
| `Solar` | | ELECON `GEMON.c` | `mSolarValid` | global_energy |
| `Load` | | ELECON `GEMON.c` | `mLoadValid` | global_energy |
| `LoadCurrent` | | ELECON `GEMON.c` | `mLoadA100Valid` (load && battery) | global_energy |
| `Mppt` | | ELECON `MPPT.c` | always 1 | mppt (+ `CHARGING_A` from global_energy) |
| `PackWorkshop251Ah` | `Pack` | ELECON_D `JKBMS.c` (today LEMON) | `validflag` = 1, only `if (Active)` | bms1 |
| `PackHome220Ah` | `Pack` | ELECON `BMS.c` UART2 (today LEMON) | same | bms2 |
| `PackHome80Ah` | `Pack` | ELECON `BMS.c` UART3 (today LEMON) | same | bms3 |
| `ShuntWorkshop` | `Shunt` | ELECON_D `SHUNT.c` | own flag | shunt |
| `ShuntHome` | `Shunt` | ELECON `SHUNT.c` | own flag | shunt |
| `StorageHome` | `Storage` | ELECON `LEMON.c` | `validflag` = 1 always | storage1 |
| `LoadHome` | `LocalLoad` | ELECON `LEMON.c` | `validflag` = 1 always | storage1 |
| `PvHome` | `LocalPv` | ELECON `LEMON.c` | `validflag` = 1 always | storage1 |
| `StorageWorkshop` | `Storage` | ELECON_D `LEMON.c` | `strgvalid` (capacity: `validflag`, see Q4) | storage2 |
| `LoadWorkshop` | `LocalLoad` | ELECON_D `LEMON.c` | `strgvalid` | storage2 |
| `PvWorkshop` | `LocalPv` | ELECON_D `LEMON.c` | `validflag` = 1 always | storage2 |
| `Axpert` | | ELECON_D `AXPERT.c` | `validflag`, effectively always 1 (Q5) | axpert |
| `MetersElecon` | | ELECON `ELM.c` (by id) | `per_field`, always 1 | el_meters |
| `MetersIoboardD` | | IOBOARD_D `ELM.c` (by id) + `APP.c` kitchen sum | `per_field`, always 1 | el_meters |
| `MetersEleconD` | | ELECON_D `ELM.c` (by id) | `per_field`, always 1 | el_meters |
| `ElHeater` | | TECHM `ELHEATER.c` | always 1 | el_heater |
| `Tank` | | TECHM `TEMPCON.c` | always 1 | heating |
| `Boiler` | | TECHM `HEATING.c` | always 1 | heating |
| `Water` | | TECHM `WM.c` | always 1 | water |
| `RecuAir` | | RECU `TEMP.c` + `SENSIRION.c` (by id) | `per_field`, per sensor | recu |
| `RecuFans` | | RECU `RECON.c` | always 1 | recu |
| `TempTechm` | | TECHM `TEMP.c` (by id) + `PTCTEMP.c` | `per_field`, per sensor | temperatures |
| `TempIoboardD` | | IOBOARD_D `TEMP.c` | `per_field` | temperatures |
| `TempIoboardU` | | IOBOARD_U `TEMP.c` | `per_field` | temperatures |
| `TempElecon` | | ELECON `TEMP.c` | `per_field` | temperatures |
| `TempEleconD` | | ELECON_D `TEMP.c` | `per_field` | temperatures |
| `Meteo` | | IOBOARD_U `METEO.c` | always 1 | meteo |

The by-id groups (meters, temperatures) are named after the node, because they are "what is wired to
this board". Energy groups are named after the place (Home / Workshop), as agreed for packs and storages.

### Variables (ids and `VID_*` unchanged)

| Id | `VID_*` (unchanged) | New group | Field |
|---|---|---|---|
| 1 | NETWORK_STATUS | System | `NetworkStatus` (uint32) |
| 10 | BAT_SOC | Battery | `Soc_pct` |
| 11 | BAT_VOLTAGE_V | Battery | `Voltage_V` |
| 14 | BAT_CURRENT_A | Battery | `Current_A` |
| 18 | BAT_POWER_W | Battery | `Power_W` |
| 16 | BAT_ENERGY_WH | Battery | `Energy_Wh` |
| 8 | SOLAR_POWER_W | Solar | `Power_W` |
| 9 | SOLAR_ENERGY_TODAY_WH | Solar | `EnergyToday_Wh` |
| 17 | LOAD_W | Load | `Power_W` |
| 15 | CONS_TODAY_WH | Load | `ConsToday_Wh` |
| 12 | LOAD_A | LoadCurrent | `Current_A` |
| 40 | MPPT_BAT_CURRENT_A | Mppt | `BatCurrent_A` |
| 41 | MPPT_BAT_VOLTAGE_V | Mppt | `BatVoltage_V` |
| 42 | MPPT_YIELD_TODAY_WH | Mppt | `YieldToday_Wh` |
| 43 | MPPT_MAX_TODAY_W | Mppt | `MaxPowerToday_W` |
| 44 | MPPT_SOLAR_POWER_W | Mppt | `SolarPower_W` |
| 45 | MPPT_SOLAR_VOLTAGE_V | Mppt | `SolarVoltage_V` |
| 46 | MPPT_SOLAR_CURRENT_A | Mppt | `SolarCurrent_A` |
| 47 | MPPT_SOLAR_MAX_VOLTAGE_V | Mppt | `SolarMaxVoltage_V` |
| 48 | MPPT_MAX_BAT_CURRENT_A | Mppt | `MaxBatCurrent_A` |
| 49 | MPPT_TEMP_C | Mppt | `Temp_C` |
| 13 | CHARGING_A | Mppt | `Charging_A` |
| 20 / 30 / 35 | BMSn_SOC | Pack* | `Soc_pct` |
| 21 / 31 / 36 | BMSn_CURRENT_A | Pack* | `Current_A` |
| 22 / 32 / 37 | BMSn_VOLTAGE_V | Pack* | `Voltage_V` |
| 23 / 33 / 38 | BMSn_ENERGY_STORED_WH | Pack* | `EnergyStored_Wh` |
| 24 / 34 / 39 | BMSn_TODAY_ENERGY_WH | Pack* | `ChargedToday_Wh` (BMS `TodayCharging_Wh`) |
| 180–195 / 220–235 / 350–365 | BMSn_CELL_V_1..16 | Pack* | `CellVoltage_V[16]` |
| 196–211 / 236–251 / 366–381 | BMSn_CELL_TEMP_C_1..16 | Pack* | `CellTemp_C[16]` |
| 50 | SHUNT_PCK1_CURRENT_A | ShuntWorkshop | `Current_A` |
| 51 | SHUNT_PCK2_CURRENT_A | ShuntHome | `Current_A` |
| 300 / 320 | STRGn_SOC | StorageHome / StorageWorkshop | `Soc_pct` |
| 301 / 321 | STRGn_POWER_W | Storage* | `Power_W` |
| 302 / 322 | STRGn_CURRENT_A | Storage* | `Current_A` |
| 303 / 323 | STRGn_ENERGY_WH | Storage* | `Energy_Wh` |
| 304 / 324 | STRGn_OPT_CHARGING_A | Storage* | `OptCharging_A` |
| 305 / 325 | STRGn_CAPACITY_AH | Storage* | `Capacity_Ah` |
| 306 / 326 | STRGn_BALANCED_TODAY | Storage* | `BalancedToday` (bool) |
| 307 / 327 | STRGn_VOLTAGE_V | Storage* | `Voltage_V` |
| 308 / 328 | LLOADn_POWER_W | LoadHome / LoadWorkshop | `Power_W` |
| 309 / 329 | LLOADn_CONS_WH | Load* | `Cons_Wh` |
| 310 / 330 | LPVn_POWER_W | PvHome / PvWorkshop | `Power_W` |
| 311 / 331 | LPVn_PROD_WH | Pv* | `Prod_Wh` |
| 278 | AXPERT_TEMP_C | Axpert | `Temp_C` |
| 279 | AXPERT_AC_POWER_W | Axpert | `AcPower_W` |
| 280 | AXPERT_BAT_CHARGING_A | Axpert | `BatCharging_A` |
| 281 | AXPERT_BAT_DISCHARGING_A | Axpert | `BatDischarging_A` |
| 282 | AXPERT_BAT_VOLTAGE_V | Axpert | `BatVoltage_V` |
| 283 | AXPERT_PVS1_W | Axpert | `Pv1Power_W` |
| 284 | AXPERT_PVS1_V | Axpert | `Pv1Voltage_V` |
| 285 | AXPERT_PVS1_A | Axpert | `Pv1Current_A` |
| 286 | AXPERT_PVS2_W | Axpert | `Pv2Power_W` |
| 287 | AXPERT_PVS2_V | Axpert | `Pv2Voltage_V` |
| 288 | AXPERT_PVS2_A | Axpert | `Pv2Current_A` |
| 290 | AXPERT_BAT_POWER_W | Axpert | `BatPower_W` |
| 291 | AXPERT_BAT_CURRENT_A | Axpert | `BatCurrent_A` |
| 292 | AXPERT_SOLAR_W | Axpert | `SolarPower_W` |
| 293 | AXPERT_ENERGY_TODAY_WH | Axpert | `EnergyToday_Wh` |
| 294 | AXPERT_LOAD_W | Axpert | `LoadPower_W` |
| 60 / 70 | CONS_AC300_WH / POW_AC300_W | MetersElecon | `Ac300Cons_Wh` / `Ac300Power_W` |
| 61 / 71 | CONS_AC3KW_WH / POW_AC3KW_W | MetersElecon | `Ac3kwCons_Wh` / `Ac3kwPower_W` |
| 62 / 72 | CONS_AC5KW_WH / POW_AC5KW_W | MetersElecon | `Ac5kwCons_Wh` / `Ac5kwPower_W` |
| 63 / 73 | CONS_FRIDGE_WH / POW_FRIDGE_W | MetersIoboardD | `FridgeCons_Wh` / `FridgePower_W` |
| 64 / 74 | CONS_KITCHEN_WH / POW_KITCHEN_W | MetersIoboardD | `KitchenCons_Wh` / `KitchenPower_W` (sum A+B, `APP.c`) |
| 65 / 75 | CONS_WASCHMACHINE_WH / POW_WASCHMACHINE_W | MetersIoboardD | `WashingMachineCons_Wh` / `WashingMachinePower_W` |
| 66 / 76 | CONS_OTHER_WH / POW_OTHER_W | MetersIoboardD | `OtherCons_Wh` / `OtherPower_W` |
| 67 / 77 | CONS_TECHM_WH / POW_TECHM_W | MetersIoboardD | `TechmCons_Wh` / `TechmPower_W` |
| 68 / 78 | CONS_KITCHEN_A_WH / POW_KITCHEN_A_W | MetersIoboardD | `KitchenACons_Wh` / `KitchenAPower_W` |
| 69 / 79 | CONS_KITCHEN_B_WH / POW_KITCHEN_B_W | MetersIoboardD | `KitchenBCons_Wh` / `KitchenBPower_W` |
| 260 / 270 | CONS_AXPERT_WH / POW_AXPERT_W | MetersEleconD | `AxpertCons_Wh` / `AxpertPower_W` |
| 261 / 271 | CONS_EVSE_WH / POW_EVSE_W | MetersEleconD | `EvseCons_Wh` / `EvsePower_W` |
| 262 / 272 | CONS_WS_HEATING_WH / POW_WS_HEATING_W | MetersEleconD | `HeatingCons_Wh` / `HeatingPower_W` |
| 80 | EL_HEATER_STATUS | ElHeater | `State` (enum `ElHeaterState`) |
| 81 | EL_HEATER_LOAD_A | ElHeater | `Load_A` |
| 82 | EL_HEATER_CURRENT_A | ElHeater | `Current_A` |
| 83 | EL_HEATER_CONS_WH | ElHeater | `ConsToday_Wh` |
| 85 | HEAT_TOTAL_WH | Tank | `TotalEnergy_Wh` |
| 86 | HEAT_HEATING_WH | Tank | `AvailableEnergy_Wh` (`mAvailableEnergyWh`) |
| 98 | BOILER_POWER_W | Boiler | `Power_W` |
| 99 | BOILER_HEAT | Boiler | `HeatToday_Wh` |
| 90 | FLOW_COLD_LPM | Water | `ColdFlow_lpm` |
| 91 | FLOW_HOT_LPM | Water | `HotFlow_lpm` |
| 92 | CONS_COLD_L | Water | `ColdCons_l` |
| 93 | CONS_HOT_L | Water | `HotCons_l` |
| 128 | TEMP_RECU_FC_C | RecuAir | `FreshCold_C` |
| 129 | TEMP_RECU_FH_C | RecuAir | `FreshHot_C` |
| 130 | TEMP_RECU_WH_C | RecuAir | `WasteHot_C` |
| 131 | TEMP_RECU_WC_C | RecuAir | `WasteCold_C` |
| 132 | RH_RECU_FH | RecuAir | `FreshHotRh_pct` |
| 133 | RH_RECU_WASTE | RecuAir | `WasteHotRh_pct` |
| 137 | CO2_RECU | RecuAir | `Co2_ppm` |
| 138 | DP_RECU_F | RecuAir | `FreshDp_Pa` |
| 139 | DP_RECU_WASTE | RecuAir | `WasteDp_Pa` |
| 140 | RECU_FAN_F | RecuFans | `Fresh_pct` |
| 141 | RECU_FAN_WASTE | RecuFans | `Waste_pct` |
| 100 | TEMP_BOILER_C | TempTechm | `Boiler_C` |
| 101 | TEMP_BOILER_IN_C | TempTechm | `BoilerIn_C` |
| 102 | TEMP_BOILER_OUT_C | TempTechm | `BoilerOut_C` |
| 103 | TEMP_TANK_IN_H_C | TempTechm | `TankInHot_C` |
| 104 | TEMP_TANK_OUT_H_C | TempTechm | `TankOutHot_C` |
| 105–110 | TEMP_TANK_1_C .. TEMP_TANK_6_C | TempTechm | `Tank1_C` .. `Tank6_C` (scalars, so `VID_*` stay) |
| 111 | TEMP_WALL_IN_C | TempTechm | `WallIn_C` |
| 112 | TEMP_WALL_OUT_C | TempTechm | `WallOut_C` |
| 113 | TEMP_BOILER_EXHAUST_C | TempTechm | `BoilerExhaust_C` (`PTCTEMP.c`, `VarSet()`) |
| 114 | TEMP_RAD_H_C | TempTechm | `RadHot_C` |
| 115 | TEMP_RAD_C_C | TempTechm | `RadCold_C` |
| 116 | TEMP_TANK_IN_C_C | TempTechm | `TankInCold_C` |
| 117 | TEMP_TANK_OUT_C_C | TempTechm | `TankOutCold_C` |
| 120 | TEMP_TECHM_BOARD_C | TempTechm | `Board_C` |
| 121 | TEMP_IOBOARD_D_C | TempIoboardD | `Board_C` |
| 124 | TEMP_DOWNSTAIRS_C | TempIoboardD | `Downstairs_C` |
| 122 | TEMP_IOBOARD_U_C | TempIoboardU | `Board_C` |
| 123 | TEMP_ELECON_BOARD_C | TempElecon | `Board_C` |
| 125 | TEMP_OFFICE_C | TempElecon | `Office_C` |
| 126 | TEMP_KIDROOM_C | TempElecon | `Kidroom_C` |
| 127 | TEMP_OUTSIDE_C | TempElecon | `Outside_C` |
| 144 | TEMP_DILNA_C | TempEleconD | `Workshop_C` |
| 145 | TEMP_AKUPACK1_C | TempEleconD | `PackWorkshop_C` |
| 146 | TEMP_BOARD_ELECON_D_C | TempEleconD | `Board_C` |
| 147 | TEMP_KOLARNA_C | TempEleconD | `BikeRoom_C` |
| 161 | METEO_WIND_BURST | Meteo | `WindBurst_mps` |
| 162 | METEO_WIND_AVG | Meteo | `WindAvg_mps` |
| 163 | METEO_WIND_POW | Meteo | `WindPower_W` |
| 164 | METEO_WIND_ENERGY | Meteo | `WindEnergy_Wh` |

### Questions for Martin

1. **Keep `name` / `VID_*` / `uha2.json` names, add a `field:` key.** Recommended: the task says keep the
   `VID_*` ids, and this way UhaGatePy and the PC tool see no name change at all. The alternative (rename
   `name` to the new style) changes every `VID_*` and `uha2.json`.
2. **`validity: per_field` for by-id groups** (meters, temperatures, `RecuAir`). Each 1-Wire / Sensirion
   sensor has its own validity today (5 errors → invalid), and that must stay. This is an exception to "one
   validity per group"; producer stays one node per group. OK?
3. **`LoadCurrent` as its own one-field group** (`mLoadA100Valid` = load && battery). Alternative: put
   `LOAD_A` into `Load` and accept that it becomes valid with only the load valid (behaviour change for the
   web, `Current_A` would need battery voltage anyway). Recommended: own group.
4. **`StorageWorkshop.Capacity_Ah`** is published with `validflag` = 1 today, the rest with `strgvalid`. In
   one group it gets `strgvalid`. No effect on ELECON `GEMON.c`: it reads capacity together with the other
   STRG2 fields into the same `invBat`, and `ENERGY_WH` is already `strgvalid`. Only the web shows capacity
   invalid while the workshop BMS is down. OK?
5. **Axpert: one group.** `AXPERT.c` has three sources (QPIGS, QPIGS2, calculated values), but the `validflag
   = 0` branches `return` before writing, so everything is always published valid; a silent Axpert freezes as
   valid. One group validity as agreed; in ELECON_D Part B I would report (not change) that it should go
   invalid when the Axpert stops answering.
6. **Field names** to confirm: `BikeRoom_C` (kolárna), `Workshop_C` (dílna), `PackWorkshop_C` (akupack1),
   `HeatingCons_Wh` (WS_HEATING), `Tank.AvailableEnergy_Wh` (HEAT_HEATING_WH), `ChargedToday_Wh` (BMS
   TODAY_ENERGY), node-named `Temp*` / `Meters*` groups.

### Findings beyond the task's list (for Part A.3/A.4 and Part B)

- **More by-id users** than listed:
  - ELECON and TECHM `SCOM.c` `CMD_SET_VAR_VALUE`: writes any id from serial as legacy raw int16
    (`data1 & 0x8000` = invalid). It needs `VARS2_SetById()` with legacy scaling, or it goes. Which?
  - TECHM `SCOM.c:223` reads any id for the PC scan list → needs a by-id read (`VARS2_GetById()`).
  - IOBOARD_D and IOBOARD_U `SCOM.c`: `SCOM_Init()` and `SCOM_Update_10ms()` are commented out, the file is
    compiled but never runs. Candidate for removal as dead code in their Part B.
  - `NETWORK_STATUS` is written in `SCOM.c` of IOBOARD_D, IOBOARD_U and TECHM, but ELECON is its producer:
    those writes are ignored by the wrapper (and two of them never run).
- `MPPT_BAT_CURRENT_A` (40) and `CHARGING_A` (13) are the same value from `MPPT.c` (`u16` twice).
- ELECON `LEMON.c` publishes `StorageHome`/`LoadHome`/`PvHome` always valid (`validflag` = 1); unlike
  ELECON_D it has no `StorageValid`. To report in ELECON Part B.
- ELECON `MPPT.c` publishes everything always valid; a silent Victron freezes as valid.
- ELECON_D `TotalCapacity_Ah = Pack1_Ah + Pack2_Ah` includes the unused slot; check when the slot is removed.
