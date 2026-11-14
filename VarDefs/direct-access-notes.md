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

## Part A — results (2026-11-14)

### Engine and API (UHA_COMMON)

- `Vars2`: every field `{value, valid}` (`sVarF32` / `sVarI32` / `sVarU32`, 8 B). `VarMeta.pVar` points to
  the field; `mState` (private in `VARS2.c`) keeps only `RxTick` / `TxTick`.
- TX: VARS2 sends every own `on_can` variable by `period_ms` (1 CAN frame per 10 ms, round robin, frame hook),
  `VARS2_UpdateLocal_10ms(cb)` sends own `on_can: false` variables to the tunnel (unchanged API).
- RX: value + valid from the frame; own ids and unknown ids are not stored (still passed to the hook);
  `VARS2_Update_10ms()` clears `.valid` after `timeout_ms`.
- Producer API: `VARS2_SetGroupValid(pGroup, valid)` (group or whole container), `VarSet(&field, v, valid)`,
  `VARS2_SetById(VID_x, value, valid)` (own variables only).
- `VARS2_Init()` sets all values 0 / invalid; it keeps the frame hook (ELECON calls `SCOM_Init()`, which
  installs it, before `COM_Init()`).
- Temporary (removed in part C): `VAR_SetVariable()` / `VAR_GetVariable()` wrappers (`LegacyScale` as
  before), `VAR_Init()` / `VAR_EnableVars2Read()` no-ops, `COM_AddStreamedVariable()` no-op,
  `COM_SendVariable()` → `VARS2_SendLegacy()` (own variables only). `Vars[]`, `sVar`, `NUM_OF_VARIABLES`,
  `VAR_GetVariableLegacy()`, `VAR_GetVariablePointer()`, `VARS2_IsValid()`, `VARS2_SetValid()`,
  `VARS2_ProcessFrame()`, `STREAM_LIST_SIZE` deleted (no users).
- Shared modules: `TEMP_AssignSensor()`, `ELM_AddMeter()`, `sI2cSensor.VarId_1/2` take `eVarId`; values by id
  in physical units. ELM `Wh` / `W` are `uint16_t`: the old wrapper path wrapped values > 32767 to negative,
  now they are sent correctly (no difference below 32768). `METEO.c` moves with IOBOARD_U part B (only user;
  `METEO_Init()` gets the group pointer, which changes the IOBOARD_U call).

### Per node: old API calls the wrappers treat differently

| Node | Call | Effect |
|---|---|---|
| TECHM | `APP.c:209` `VAR_SetVariable(VAR_BAT_VOLTAGE_V10, 520, 1)` | ignored (ELECON produces it); removed in TECHM part B |
| TECHM | `SCOM.c:82` `NETWORK_STATUS` | ignored; SCOM removed in part B |
| TECHM, ELECON | `SCOM.c` `CMD_SET_VAR_VALUE` (`data1 & 0x7FFF`) | own ids set as before, other ids ignored; removed in part B |
| IOBOARD_D, IOBOARD_U | `SCOM.c:82` `NETWORK_STATUS` | ignored; SCOM never runs (init commented out), removed in part B |

No variable is written before `COM_Init()` on any node (checked the `APP_Init()` order), so `VARS2_Init()`
clearing `Vars2` loses nothing.

### Per node: YAML `period_ms` vs `COM_AddStreamedVariable()`

All periods identical on all six nodes, and every own `on_can` variable was streamed before
(IOBOARD_D 12, IOBOARD_U 5, TECHM 31, ELECON 10, RECU 11, ELECON_D 76 variables). The YAML-driven TX sends
exactly the same set at the same periods. Only the order of the frames within a period differs (round robin
instead of list order).

### Concurrency (all Vars2 access in the main loop?)

- CAN RX: `COM_GetRxMessage()` in the main loop on all nodes; nothing in `stm32*_it.c` touches variables.
- BMS UART callbacks (ELECON, ELECON_D `LEMON_UartRxCallback()` → `BMS_UartRxCallback()`): no variables.
- **TECHM `WM.c` `WM_ExtiCallback()` (from `HAL_GPIO_EXTI_Callback()`, interrupt) calls
  `VAR_SetVariable(VAR_CONS_COLD / VAR_CONS_HOT)`.** Question for Martin (TECHM part B); proposal: the
  callback only counts (`mConsCold += 3`), `WM_Update` in the main loop publishes (it does already).
- **TECHM `SCOM.c` `HAL_UART_RxCpltCallback()` calls `ProcessMessage()` in the UART interrupt**
  (`CMD_SET_VAR_VALUE` → `VAR_SetVariable()`, scan list → `VAR_GetVariable()`), found by the reviewer.
  Goes away with SCOM in TECHM part B (decision 7). IOBOARD_D has the same pattern, but its SCOM never runs.

### Check builds (CubeIDE GCC 13.3, `make all`, Deployment restored, version not bumped)

| Node | Warnings main → branch | text main → branch | bss main → branch |
|---|---|---|---|
| IOBOARD_U | 21 → 21 | 46512 → 46808 (+296) | 42808 → 41032 (−1776) |
| IOBOARD_D | 21 → 21 | 63428 → 63644 (+216) | 42640 → 40872 (−1768) |
| RECU | 37 → 37 * | 65236 → 65312 (+76) | 9552 → 7784 (−1768) |
| ELECON_D | 54 → 54 | 109420 → 109628 (+208) | 22744 → 20976 (−1768) |
| TECHM | 28 → 28 | 60132 → 60220 (+88) | 10728 → 8960 (−1768) |
| ELECON | 30 → 30 | 90896 → 91000 (+104) | 23544 → 21256 (−2288) |

\* RECU compiles its own `UhaCommon/Src` copies but with `../../UHA_COMMON/Inc` first in the include path,
so with the new shared headers it does **not** build until its `UhaCommon/` is synced (RECU part B step 1).
Built in a scratch copy with the sync applied: OK (numbers above). The RECU repo is untouched.

### PC tests (`VarDefs/tests`)

`run_validator_tests` 72/72, `compile_check`, `test_vars2` (incl. wrapper round trip: 258 rows × 65536 raw
values, 0 mismatches), `test_com` (YAML TX per node), `test_compat` (old `main` vs branch, 6 nodes, 9282 lines:
CAN frames, tunnel frames, own read back, received reads, timeouts — identical), `test_tunnel`, `test_mcan`:
all pass.

### YAML readers outside the firmware (not changed, task A.8)

The ids and `name`s did not change, so `uha.json` / `uha2.json` contents stay the same. But both readers
walk `groups.<g>.vars` only; 101 of 258 values are now in `subgroups` (`GlobalEnergy`, `HomeEnergy`,
`WorkshopEnergy`, `Meters`, `Recu`, `Temperatures`):

- **UhaGatePy** `uhagatepy/vars_def.py` `_build()`: `group["vars"]` → `KeyError` → `VarsDefError`, the gateway
  refuses to start. `deploy.sh` copies the YAML from the UHA_FW working tree: do not deploy before the
  loader reads `subgroups` (producer from the subgroup, else the container).
- **UhaCanMaster** `UhaCanCore/Decoding/VarCatalog.cs` `Parser`: groups without `vars` are skipped without
  a warning, so those 101 values show as unknown ids. It reads the YAML from the UHA_FW working tree, i.e.
  already now while `UHA_COMMON` is on this branch. `VarDefinition.Group` holds the group key (now
  `GlobalEnergy` etc. / the subgroup key): display only.

### Independent review (part A.9)

No wire-format or engine bug. Verified independently: YAML metadata of all 258 values, VarMeta rows and
VarIdToIndex identical to `main` (except `pVar`), stream periods per node, wrapper equivalence, tick wrap,
`VARS2_SetGroupValid()` layout checks, shared-module conversions. Findings and what was done:

1. TECHM SCOM UART callback writes variables in the interrupt → noted above (TECHM part B removes SCOM).
2. RECU does not build until `UhaCommon/` is synced → RECU part B step 1 first (already planned).
3. IOBOARD_D `APP.c:268-273` sums kitchen A + B as int16 from `VAR_GetVariable()`: above 32767 Wh the old
   code wrapped, the wrapper now saturates (both wrong, different). IOBOARD_D part B: sum in float.
4. `VarSet()` only for float → added `VarSetI32()` / `VarSetU32()`.
5. `VARS2_SetGroupValid()` without a type check → compile-time check that the size is a multiple of 8;
   comment: only for own `validity: group` groups (per_field groups via `VarSet*()` / `VARS2_SetById()`).
6. No test across the tick wrap → `test_tick_wrap` in `test_vars2.c` (timeout and TX across 2^32).
   `test_compat` compares the last frame per id and phase (not counts / order); periods are checked by
   `test_com` against the YAML and by the per-node script above.
7. Dead `seen_member` in the validator → removed.
8. Writes before `COM_Init()` are silently ignored (no node does it today).

## Part B — IOBOARD_U (node 2), v001.04

Branches: `ASW_IOBOARD_U` `feature/vars2_direct_access`, `UHA_COMMON` `feature/vars2_direct_access_ioboard_u`
(METEO; from `main` with part A).

### Inventory (old API, before)

| Where | Call | Class | Now |
|---|---|---|---|
| `APP.c:199` | `TEMP_AssignSensor(T_IOBOARD_U, VAR_TEMP_IOBOARD_U, 0)` | by id | `VID_TEMP_IOBOARD_U_C` |
| `APP.c:202-206` | `COM_AddStreamedVariable()` × 5 | TX config | removed (YAML: same 5 variables, same periods) |
| `APP.c:55` | `VAR_Init()` | — | removed |
| `APP.c:61` | `METEO_Init(IMP1_Pin, IMP2_Pin)` | produced (`Meteo`) | `METEO_Init(..., &Vars2.Meteo)` |
| `METEO.c:64-67` | `VAR_SetVariable(VAR_METEO_*)` × 4 | produced | `mpVars->Wind*.value`, `VARS2_SetGroupValid(mpVars, 1)` |
| `SCOM.c` | `VAR_SetVariable/GetVariable`, `NETWORK_STATUS` | by id | file removed (excluded from the build, never initialised) |

Reads: none (IOBOARD_U reads no other node's variables).

### Constants

No threshold or constant compares a variable on this node. METEO keeps measuring in dm/s internally; the
published values are converted: `WindBurst_mps = WindSpeedBurst / 10`, `WindAvg_mps = WindSpeedAvg / 10`
(`legacy_scale` 10 as before, so the frames are unchanged).

### Removed dead code

- `SCOM.c`, `SCOM.h` (+ includes / commented calls in `APP.c`, `scheduler.c`). The `.cproject` still has the
  build exclusion entry for `Src/SCOM.c` (harmless, CubeIDE file left untouched).
- `sWindSensor.VarId`, `sRainSensor.VarId` in `METEO.h` (never used).

### Concurrency

`METEO_ExtiCallback()` (EXTI interrupt) writes only `mWind`; `Vars2.Meteo` is written in `METEO_Update_1s()`
(main loop). Nothing else touches variables in an interrupt.

### Build

v001.04, CRC `0xCAADF741`, 21 warnings (as before, all in older code). text 46512 → 47308 (+796 B),
data 544 → 544, bss 42808 → 41032 (−1776 B), before = v001.03 code with `UHA_COMMON` before part A.
