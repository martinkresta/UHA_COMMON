# VARS2 direct access — notes

Task: `UHA_COMMON/tasks/vars2_direct_access.md`. Branch `feature/vars2_direct_access`.

## Part A.1 — YAML layout (agreed with Martin 2026-11-14)

Ids, `name` (so `VID_*` and `uha2.json` names), types, `legacy`, `legacy_scale`, `period_ms`, `timeout_ms`,
`on_can` and producers are unchanged for all 258 values (checked against the YAML on `main`).

### Schema

| Key | Where | Meaning |
|---|---|---|
| group key | group / subgroup | CamelCase C member name: `Vars2.<Group>.<Field>` or `Vars2.<Group>.<Subgroup>.<Field>`, max 3 levels |
| `vars` / `subgroups` | group | a group holds fields, or is a container of subgroups (subgroups hold fields only) |
| `producer` | group or subgroup | one producer per group with fields; never on a variable; not on both container and subgroup |
| `struct` | top-level group | shared C type `sVars<struct>`; all groups with it must have identical fields |
| `validity` | group with fields | `group` (default): `VARS2_SetGroupValid()`; `per_field`: fields written by id (TEMP, ELM, SENSIRION), each with its own validity |
| `field` | variable, required | C field name, unit suffix required when `unit` is set: `_pct _V _A _W _Wh _Ah _C _l _lpm _Pa _ppm _mps`; none for bool / enum |

A container gives one pointer per producer module (`GEMON_Init(&Vars2.GlobalEnergy, ...)`), its subgroups
are the validity units (`VARS2_SetGroupValid(&p->Battery, valid)`). Namespace containers (`Meters`,
`Temperatures`) have one producer per subgroup.

### Layout

```
Vars2.System              ELECON SCOM
Vars2.GlobalEnergy        ELECON GEMON    { Battery, Solar, Load }        Load incl. LOAD_A (Current_A)
Vars2.PackWorkshop251Ah   ELECON_D JKBMS  sVarsPack
Vars2.PackHome220Ah       ELECON BMS      sVarsPack (UART2)
Vars2.PackHome80Ah        ELECON BMS      sVarsPack (UART3)
Vars2.Mppt                ELECON MPPT     incl. Charging_A
Vars2.ShuntWorkshop       ELECON_D SHUNT  sVarsShunt
Vars2.ShuntHome           ELECON SHUNT    sVarsShunt
Vars2.HomeEnergy          ELECON LEMON    sVarsLocalEnergy { Storage, Load, Pv }
Vars2.WorkshopEnergy      ELECON_D LEMON  sVarsLocalEnergy { Storage, Load, Pv }
Vars2.Axpert              ELECON_D AXPERT
Vars2.Meters              namespace       { Elecon, IoboardD, EleconD }                  per_field
Vars2.ElHeater            TECHM ELHEATER
Vars2.Tank                TECHM TEMPCON
Vars2.Boiler              TECHM HEATING
Vars2.Water               TECHM WM
Vars2.Recu                RECU            { Air (TEMP + SENSIRION, per_field), Fans (RECON) }
Vars2.Temperatures        namespace       { Techm, IoboardD, IoboardU, Elecon, EleconD }  per_field
Vars2.Meteo               IOBOARD_U METEO
```

### Decisions

1. `name` / `VID_*` unchanged, new `field:` key (Martin will redeploy UhaGatePy and the PC tool anyway).
2. `validity: per_field` for by-id groups (each 1-Wire / Sensirion sensor keeps its own validity).
3. `LOAD_A` is `GlobalEnergy.Load.Current_A` with the load validity. GEMON can compute it only with a valid
   battery voltage; while the battery is invalid and the load valid it keeps the last value (ELECON Part B).
4. `WorkshopEnergy.Storage.Capacity_Ah` gets the storage validity (was always valid). No effect on GEMON.
5. Axpert stays one group; a silent Axpert freezes as valid today: report in ELECON_D Part B, no change.
6. Field names as in the table below (incl. `BikeRoom_C`, `Workshop_C`, `PackWorkshop_C`,
   `HeatingCons_Wh`, `Tank.AvailableEnergy_Wh`, `ChargedToday_Wh`).
7. SCOM is obsolete: remove `SCOM.c` in TECHM, IOBOARD_D, IOBOARD_U (their Part B); in ELECON remove only
   the `CMD_SET_VAR_VALUE` case (tunnel stays). No by-id read / serial write API in the engine.
8. Superstruct (3 levels) agreed; level-2 names `HomeEnergy` / `WorkshopEnergy`.

### Variables (ids and `VID_*` unchanged)

| Id | `VID_*` | `Vars2.` field |
|---|---|---|
| 1 | NETWORK_STATUS | `System.NetworkStatus` (uint32) |
| 10 | BAT_SOC | `GlobalEnergy.Battery.Soc_pct` |
| 11 | BAT_VOLTAGE_V | `GlobalEnergy.Battery.Voltage_V` |
| 14 | BAT_CURRENT_A | `GlobalEnergy.Battery.Current_A` |
| 16 | BAT_ENERGY_WH | `GlobalEnergy.Battery.Energy_Wh` |
| 18 | BAT_POWER_W | `GlobalEnergy.Battery.Power_W` |
| 8 | SOLAR_POWER_W | `GlobalEnergy.Solar.Power_W` |
| 9 | SOLAR_ENERGY_TODAY_WH | `GlobalEnergy.Solar.EnergyToday_Wh` |
| 12 | LOAD_A | `GlobalEnergy.Load.Current_A` |
| 15 | CONS_TODAY_WH | `GlobalEnergy.Load.ConsToday_Wh` |
| 17 | LOAD_W | `GlobalEnergy.Load.Power_W` |
| 20 | BMS1_SOC | `PackWorkshop251Ah.Soc_pct` |
| 21 | BMS1_CURRENT_A | `PackWorkshop251Ah.Current_A` |
| 22 | BMS1_VOLTAGE_V | `PackWorkshop251Ah.Voltage_V` |
| 23 | BMS1_ENERGY_STORED_WH | `PackWorkshop251Ah.EnergyStored_Wh` |
| 24 | BMS1_TODAY_ENERGY_WH | `PackWorkshop251Ah.ChargedToday_Wh` |
| 180–195 | BMS1_CELL_V_1..16 | `PackWorkshop251Ah.CellVoltage_V[16]` |
| 196–211 | BMS1_CELL_TEMP_C_1..16 | `PackWorkshop251Ah.CellTemp_C[16]` |
| 30 | BMS2_SOC | `PackHome220Ah.Soc_pct` |
| 31 | BMS2_CURRENT_A | `PackHome220Ah.Current_A` |
| 32 | BMS2_VOLTAGE_V | `PackHome220Ah.Voltage_V` |
| 33 | BMS2_ENERGY_STORED_WH | `PackHome220Ah.EnergyStored_Wh` |
| 34 | BMS2_TODAY_ENERGY_WH | `PackHome220Ah.ChargedToday_Wh` |
| 220–235 | BMS2_CELL_V_1..16 | `PackHome220Ah.CellVoltage_V[16]` |
| 236–251 | BMS2_CELL_TEMP_C_1..16 | `PackHome220Ah.CellTemp_C[16]` |
| 35 | BMS3_SOC | `PackHome80Ah.Soc_pct` |
| 36 | BMS3_CURRENT_A | `PackHome80Ah.Current_A` |
| 37 | BMS3_VOLTAGE_V | `PackHome80Ah.Voltage_V` |
| 38 | BMS3_ENERGY_STORED_WH | `PackHome80Ah.EnergyStored_Wh` |
| 39 | BMS3_TODAY_ENERGY_WH | `PackHome80Ah.ChargedToday_Wh` |
| 350–365 | BMS3_CELL_V_1..16 | `PackHome80Ah.CellVoltage_V[16]` |
| 366–381 | BMS3_CELL_TEMP_C_1..16 | `PackHome80Ah.CellTemp_C[16]` |
| 40 | MPPT_BAT_CURRENT_A | `Mppt.BatCurrent_A` |
| 41 | MPPT_BAT_VOLTAGE_V | `Mppt.BatVoltage_V` |
| 42 | MPPT_YIELD_TODAY_WH | `Mppt.YieldToday_Wh` |
| 43 | MPPT_MAX_TODAY_W | `Mppt.MaxPowerToday_W` |
| 44 | MPPT_SOLAR_POWER_W | `Mppt.SolarPower_W` |
| 45 | MPPT_SOLAR_VOLTAGE_V | `Mppt.SolarVoltage_V` |
| 46 | MPPT_SOLAR_CURRENT_A | `Mppt.SolarCurrent_A` |
| 47 | MPPT_SOLAR_MAX_VOLTAGE_V | `Mppt.SolarMaxVoltage_V` |
| 48 | MPPT_MAX_BAT_CURRENT_A | `Mppt.MaxBatCurrent_A` |
| 49 | MPPT_TEMP_C | `Mppt.Temp_C` |
| 13 | CHARGING_A | `Mppt.Charging_A` |
| 50 | SHUNT_PCK1_CURRENT_A | `ShuntWorkshop.Current_A` |
| 51 | SHUNT_PCK2_CURRENT_A | `ShuntHome.Current_A` |
| 300 | STRG1_SOC | `HomeEnergy.Storage.Soc_pct` |
| 301 | STRG1_POWER_W | `HomeEnergy.Storage.Power_W` |
| 302 | STRG1_CURRENT_A | `HomeEnergy.Storage.Current_A` |
| 303 | STRG1_ENERGY_WH | `HomeEnergy.Storage.Energy_Wh` |
| 304 | STRG1_OPT_CHARGING_A | `HomeEnergy.Storage.OptCharging_A` |
| 305 | STRG1_CAPACITY_AH | `HomeEnergy.Storage.Capacity_Ah` |
| 306 | STRG1_BALANCED_TODAY | `HomeEnergy.Storage.BalancedToday` (bool) |
| 307 | STRG1_VOLTAGE_V | `HomeEnergy.Storage.Voltage_V` |
| 308 | LLOAD1_POWER_W | `HomeEnergy.Load.Power_W` |
| 309 | LLOAD1_CONS_WH | `HomeEnergy.Load.Cons_Wh` |
| 310 | LPV1_POWER_W | `HomeEnergy.Pv.Power_W` |
| 311 | LPV1_PROD_WH | `HomeEnergy.Pv.Prod_Wh` |
| 320 | STRG2_SOC | `WorkshopEnergy.Storage.Soc_pct` |
| 321 | STRG2_POWER_W | `WorkshopEnergy.Storage.Power_W` |
| 322 | STRG2_CURRENT_A | `WorkshopEnergy.Storage.Current_A` |
| 323 | STRG2_ENERGY_WH | `WorkshopEnergy.Storage.Energy_Wh` |
| 324 | STRG2_OPT_CHARGING_A | `WorkshopEnergy.Storage.OptCharging_A` |
| 325 | STRG2_CAPACITY_AH | `WorkshopEnergy.Storage.Capacity_Ah` |
| 326 | STRG2_BALANCED_TODAY | `WorkshopEnergy.Storage.BalancedToday` (bool) |
| 327 | STRG2_VOLTAGE_V | `WorkshopEnergy.Storage.Voltage_V` |
| 328 | LLOAD2_POWER_W | `WorkshopEnergy.Load.Power_W` |
| 329 | LLOAD2_CONS_WH | `WorkshopEnergy.Load.Cons_Wh` |
| 330 | LPV2_POWER_W | `WorkshopEnergy.Pv.Power_W` |
| 331 | LPV2_PROD_WH | `WorkshopEnergy.Pv.Prod_Wh` |
| 278 | AXPERT_TEMP_C | `Axpert.Temp_C` |
| 279 | AXPERT_AC_POWER_W | `Axpert.AcPower_W` |
| 280 | AXPERT_BAT_CHARGING_A | `Axpert.BatCharging_A` |
| 281 | AXPERT_BAT_DISCHARGING_A | `Axpert.BatDischarging_A` |
| 282 | AXPERT_BAT_VOLTAGE_V | `Axpert.BatVoltage_V` |
| 283 | AXPERT_PVS1_W | `Axpert.Pv1Power_W` |
| 284 | AXPERT_PVS1_V | `Axpert.Pv1Voltage_V` |
| 285 | AXPERT_PVS1_A | `Axpert.Pv1Current_A` |
| 286 | AXPERT_PVS2_W | `Axpert.Pv2Power_W` |
| 287 | AXPERT_PVS2_V | `Axpert.Pv2Voltage_V` |
| 288 | AXPERT_PVS2_A | `Axpert.Pv2Current_A` |
| 290 | AXPERT_BAT_POWER_W | `Axpert.BatPower_W` |
| 291 | AXPERT_BAT_CURRENT_A | `Axpert.BatCurrent_A` |
| 292 | AXPERT_SOLAR_W | `Axpert.SolarPower_W` |
| 293 | AXPERT_ENERGY_TODAY_WH | `Axpert.EnergyToday_Wh` |
| 294 | AXPERT_LOAD_W | `Axpert.LoadPower_W` |
| 60 | CONS_AC300_WH | `Meters.Elecon.Ac300Cons_Wh` |
| 61 | CONS_AC3KW_WH | `Meters.Elecon.Ac3kwCons_Wh` |
| 62 | CONS_AC5KW_WH | `Meters.Elecon.Ac5kwCons_Wh` |
| 70 | POW_AC300_W | `Meters.Elecon.Ac300Power_W` |
| 71 | POW_AC3KW_W | `Meters.Elecon.Ac3kwPower_W` |
| 72 | POW_AC5KW_W | `Meters.Elecon.Ac5kwPower_W` |
| 63 | CONS_FRIDGE_WH | `Meters.IoboardD.FridgeCons_Wh` |
| 64 | CONS_KITCHEN_WH | `Meters.IoboardD.KitchenCons_Wh` |
| 65 | CONS_WASCHMACHINE_WH | `Meters.IoboardD.WashingMachineCons_Wh` |
| 66 | CONS_OTHER_WH | `Meters.IoboardD.OtherCons_Wh` |
| 67 | CONS_TECHM_WH | `Meters.IoboardD.TechmCons_Wh` |
| 68 | CONS_KITCHEN_A_WH | `Meters.IoboardD.KitchenACons_Wh` |
| 69 | CONS_KITCHEN_B_WH | `Meters.IoboardD.KitchenBCons_Wh` |
| 73 | POW_FRIDGE_W | `Meters.IoboardD.FridgePower_W` |
| 74 | POW_KITCHEN_W | `Meters.IoboardD.KitchenPower_W` |
| 75 | POW_WASCHMACHINE_W | `Meters.IoboardD.WashingMachinePower_W` |
| 76 | POW_OTHER_W | `Meters.IoboardD.OtherPower_W` |
| 77 | POW_TECHM_W | `Meters.IoboardD.TechmPower_W` |
| 78 | POW_KITCHEN_A_W | `Meters.IoboardD.KitchenAPower_W` |
| 79 | POW_KITCHEN_B_W | `Meters.IoboardD.KitchenBPower_W` |
| 260 | CONS_AXPERT_WH | `Meters.EleconD.AxpertCons_Wh` |
| 261 | CONS_EVSE_WH | `Meters.EleconD.EvseCons_Wh` |
| 262 | CONS_WS_HEATING_WH | `Meters.EleconD.HeatingCons_Wh` |
| 270 | POW_AXPERT_W | `Meters.EleconD.AxpertPower_W` |
| 271 | POW_EVSE_W | `Meters.EleconD.EvsePower_W` |
| 272 | POW_WS_HEATING_W | `Meters.EleconD.HeatingPower_W` |
| 80 | EL_HEATER_STATUS | `ElHeater.State` (enum ElHeaterState) |
| 81 | EL_HEATER_LOAD_A | `ElHeater.Load_A` |
| 82 | EL_HEATER_CURRENT_A | `ElHeater.Current_A` |
| 83 | EL_HEATER_CONS_WH | `ElHeater.ConsToday_Wh` |
| 85 | HEAT_TOTAL_WH | `Tank.TotalEnergy_Wh` |
| 86 | HEAT_HEATING_WH | `Tank.AvailableEnergy_Wh` |
| 98 | BOILER_POWER_W | `Boiler.Power_W` |
| 99 | BOILER_HEAT | `Boiler.HeatToday_Wh` |
| 90 | FLOW_COLD_LPM | `Water.ColdFlow_lpm` |
| 91 | FLOW_HOT_LPM | `Water.HotFlow_lpm` |
| 92 | CONS_COLD_L | `Water.ColdCons_l` |
| 93 | CONS_HOT_L | `Water.HotCons_l` |
| 128 | TEMP_RECU_FC_C | `Recu.Air.FreshCold_C` |
| 129 | TEMP_RECU_FH_C | `Recu.Air.FreshHot_C` |
| 130 | TEMP_RECU_WH_C | `Recu.Air.WasteHot_C` |
| 131 | TEMP_RECU_WC_C | `Recu.Air.WasteCold_C` |
| 132 | RH_RECU_FH | `Recu.Air.FreshHotRh_pct` |
| 133 | RH_RECU_WASTE | `Recu.Air.WasteHotRh_pct` |
| 137 | CO2_RECU | `Recu.Air.Co2_ppm` |
| 138 | DP_RECU_F | `Recu.Air.FreshDp_Pa` |
| 139 | DP_RECU_WASTE | `Recu.Air.WasteDp_Pa` |
| 140 | RECU_FAN_F | `Recu.Fans.Fresh_pct` |
| 141 | RECU_FAN_WASTE | `Recu.Fans.Waste_pct` |
| 100 | TEMP_BOILER_C | `Temperatures.Techm.Boiler_C` |
| 101 | TEMP_BOILER_IN_C | `Temperatures.Techm.BoilerIn_C` |
| 102 | TEMP_BOILER_OUT_C | `Temperatures.Techm.BoilerOut_C` |
| 103 | TEMP_TANK_IN_H_C | `Temperatures.Techm.TankInHot_C` |
| 104 | TEMP_TANK_OUT_H_C | `Temperatures.Techm.TankOutHot_C` |
| 105 | TEMP_TANK_1_C | `Temperatures.Techm.Tank1_C` |
| 106 | TEMP_TANK_2_C | `Temperatures.Techm.Tank2_C` |
| 107 | TEMP_TANK_3_C | `Temperatures.Techm.Tank3_C` |
| 108 | TEMP_TANK_4_C | `Temperatures.Techm.Tank4_C` |
| 109 | TEMP_TANK_5_C | `Temperatures.Techm.Tank5_C` |
| 110 | TEMP_TANK_6_C | `Temperatures.Techm.Tank6_C` |
| 111 | TEMP_WALL_IN_C | `Temperatures.Techm.WallIn_C` |
| 112 | TEMP_WALL_OUT_C | `Temperatures.Techm.WallOut_C` |
| 113 | TEMP_BOILER_EXHAUST_C | `Temperatures.Techm.BoilerExhaust_C` |
| 114 | TEMP_RAD_H_C | `Temperatures.Techm.RadHot_C` |
| 115 | TEMP_RAD_C_C | `Temperatures.Techm.RadCold_C` |
| 116 | TEMP_TANK_IN_C_C | `Temperatures.Techm.TankInCold_C` |
| 117 | TEMP_TANK_OUT_C_C | `Temperatures.Techm.TankOutCold_C` |
| 120 | TEMP_TECHM_BOARD_C | `Temperatures.Techm.Board_C` |
| 121 | TEMP_IOBOARD_D_C | `Temperatures.IoboardD.Board_C` |
| 124 | TEMP_DOWNSTAIRS_C | `Temperatures.IoboardD.Downstairs_C` |
| 122 | TEMP_IOBOARD_U_C | `Temperatures.IoboardU.Board_C` |
| 123 | TEMP_ELECON_BOARD_C | `Temperatures.Elecon.Board_C` |
| 125 | TEMP_OFFICE_C | `Temperatures.Elecon.Office_C` |
| 126 | TEMP_KIDROOM_C | `Temperatures.Elecon.Kidroom_C` |
| 127 | TEMP_OUTSIDE_C | `Temperatures.Elecon.Outside_C` |
| 144 | TEMP_DILNA_C | `Temperatures.EleconD.Workshop_C` |
| 145 | TEMP_AKUPACK1_C | `Temperatures.EleconD.PackWorkshop_C` |
| 146 | TEMP_BOARD_ELECON_D_C | `Temperatures.EleconD.Board_C` |
| 147 | TEMP_KOLARNA_C | `Temperatures.EleconD.BikeRoom_C` |
| 161 | METEO_WIND_BURST | `Meteo.WindBurst_mps` |
| 162 | METEO_WIND_AVG | `Meteo.WindAvg_mps` |
| 163 | METEO_WIND_POW | `Meteo.WindPower_W` |
| 164 | METEO_WIND_ENERGY | `Meteo.WindEnergy_Wh` |

### Findings beyond the task's list (for Part B)

- SCOM by-id users (decision 7): ELECON and TECHM `SCOM.c` `CMD_SET_VAR_VALUE` (writes any id from serial),
  TECHM `SCOM.c` scan list (reads any id), IOBOARD_D / IOBOARD_U `SCOM.c` (compiled, never initialised).
  `NETWORK_STATUS` is also written in the SCOM of IOBOARD_D, IOBOARD_U and TECHM, but ELECON produces it.
- `MPPT_BAT_CURRENT_A` (40) and `CHARGING_A` (13) are the same value from `MPPT.c` (`u16` twice).
- ELECON `LEMON.c` publishes `HomeEnergy` always valid (`validflag` = 1); unlike ELECON_D it has no
  `StorageValid`. Report in ELECON Part B.
- ELECON `MPPT.c` publishes everything always valid; a silent Victron freezes as valid.
- ELECON_D `TotalCapacity_Ah = Pack1_Ah + Pack2_Ah` includes the unused slot; check when the slot is removed.
