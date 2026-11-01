/* AUTO-GENERATED from uha_vars.yaml - DO NOT EDIT
 * YAML version: 0
 * regenerate with tools/gen_all.py
 */

#ifndef UHA_VARS_GEN_H_
#define UHA_VARS_GEN_H_

#include <stdint.h>
#include "VARS2_types.h"

#define VARS_COUNT         257
#define VAR_MAX_ID         381
#define VARS_ON_CAN_COUNT  145

/* ---- variable ids ---- */
typedef enum
{
  VID_SOLAR_POWER_W            =   8,  /* global_energy, W */
  VID_SOLAR_ENERGY_TODAY_WH    =   9,  /* global_energy, Wh */
  VID_BAT_SOC                  =  10,  /* global_energy, % */
  VID_BAT_VOLTAGE_V            =  11,  /* global_energy, V */
  VID_LOAD_A                   =  12,  /* global_energy, A */
  VID_CHARGING_A               =  13,  /* global_energy, A */
  VID_BAT_CURRENT_A            =  14,  /* global_energy, A */
  VID_CONS_TODAY_WH            =  15,  /* global_energy, Wh */
  VID_BAT_ENERGY_WH            =  16,  /* global_energy, Wh */
  VID_LOAD_W                   =  17,  /* global_energy, W */
  VID_BAT_POWER_W              =  18,  /* global_energy, W */
  VID_BMS1_SOC                 =  20,  /* bms1, % */
  VID_BMS1_CURRENT_A           =  21,  /* bms1, A */
  VID_BMS1_VOLTAGE_V           =  22,  /* bms1, V */
  VID_BMS1_ENERGY_STORED_WH    =  23,  /* bms1, Wh */
  VID_BMS1_TODAY_ENERGY_WH     =  24,  /* bms1, Wh */
  VID_BMS2_SOC                 =  30,  /* bms2, % */
  VID_BMS2_CURRENT_A           =  31,  /* bms2, A */
  VID_BMS2_VOLTAGE_V           =  32,  /* bms2, V */
  VID_BMS2_ENERGY_STORED_WH    =  33,  /* bms2, Wh */
  VID_BMS2_TODAY_ENERGY_WH     =  34,  /* bms2, Wh */
  VID_BMS3_SOC                 =  35,  /* bms3, % */
  VID_BMS3_CURRENT_A           =  36,  /* bms3, A */
  VID_BMS3_VOLTAGE_V           =  37,  /* bms3, V */
  VID_BMS3_ENERGY_STORED_WH    =  38,  /* bms3, Wh */
  VID_BMS3_TODAY_ENERGY_WH     =  39,  /* bms3, Wh */
  VID_MPPT_BAT_CURRENT_A       =  40,  /* mppt, A */
  VID_MPPT_BAT_VOLTAGE_V       =  41,  /* mppt, V */
  VID_MPPT_YIELD_TODAY_WH      =  42,  /* mppt, Wh */
  VID_MPPT_MAX_TODAY_W         =  43,  /* mppt, W */
  VID_MPPT_SOLAR_POWER_W       =  44,  /* mppt, W */
  VID_MPPT_SOLAR_VOLTAGE_V     =  45,  /* mppt, V */
  VID_MPPT_SOLAR_CURRENT_A     =  46,  /* mppt, A */
  VID_MPPT_SOLAR_MAX_VOLTAGE_V =  47,  /* mppt, V */
  VID_MPPT_MAX_BAT_CURRENT_A   =  48,  /* mppt, A */
  VID_MPPT_TEMP_C              =  49,  /* mppt, degC */
  VID_SHUNT_PCK1_CURRENT_A     =  50,  /* shunt, A */
  VID_SHUNT_PCK2_CURRENT_A     =  51,  /* shunt, A */
  VID_CONS_AC300_WH            =  60,  /* el_meters, Wh */
  VID_CONS_AC3KW_WH            =  61,  /* el_meters, Wh */
  VID_CONS_AC5KW_WH            =  62,  /* el_meters, Wh */
  VID_CONS_FRIDGE_WH           =  63,  /* el_meters, Wh */
  VID_CONS_KITCHEN_WH          =  64,  /* el_meters, Wh */
  VID_CONS_WASCHMACHINE_WH     =  65,  /* el_meters, Wh */
  VID_CONS_OTHER_WH            =  66,  /* el_meters, Wh */
  VID_CONS_TECHM_WH            =  67,  /* el_meters, Wh */
  VID_CONS_KITCHEN_A_WH        =  68,  /* el_meters, Wh */
  VID_CONS_KITCHEN_B_WH        =  69,  /* el_meters, Wh */
  VID_POW_AC300_W              =  70,  /* el_meters, W */
  VID_POW_AC3KW_W              =  71,  /* el_meters, W */
  VID_POW_AC5KW_W              =  72,  /* el_meters, W */
  VID_POW_FRIDGE_W             =  73,  /* el_meters, W */
  VID_POW_KITCHEN_W            =  74,  /* el_meters, W */
  VID_POW_WASCHMACHINE_W       =  75,  /* el_meters, W */
  VID_POW_OTHER_W              =  76,  /* el_meters, W */
  VID_POW_TECHM_W              =  77,  /* el_meters, W */
  VID_POW_KITCHEN_A_W          =  78,  /* el_meters, W */
  VID_POW_KITCHEN_B_W          =  79,  /* el_meters, W */
  VID_EL_HEATER_STATUS         =  80,  /* el_heater */
  VID_EL_HEATER_LOAD_A         =  81,  /* el_heater, A */
  VID_EL_HEATER_CURRENT_A      =  82,  /* el_heater, A */
  VID_EL_HEATER_CONS_WH        =  83,  /* el_heater, Wh */
  VID_HEAT_TOTAL_WH            =  85,  /* heating, Wh */
  VID_HEAT_HEATING_WH          =  86,  /* heating, Wh */
  VID_FLOW_COLD_LPM            =  90,  /* water, l/min */
  VID_FLOW_HOT_LPM             =  91,  /* water, l/min */
  VID_CONS_COLD_L              =  92,  /* water, l */
  VID_CONS_HOT_L               =  93,  /* water, l */
  VID_BOILER_POWER_W           =  98,  /* heating, W */
  VID_BOILER_HEAT              =  99,  /* heating, Wh */
  VID_TEMP_BOILER_C            = 100,  /* temperatures, degC */
  VID_TEMP_BOILER_IN_C         = 101,  /* temperatures, degC */
  VID_TEMP_BOILER_OUT_C        = 102,  /* temperatures, degC */
  VID_TEMP_TANK_IN_H_C         = 103,  /* temperatures, degC */
  VID_TEMP_TANK_OUT_H_C        = 104,  /* temperatures, degC */
  VID_TEMP_TANK_1_C            = 105,  /* temperatures, degC */
  VID_TEMP_TANK_2_C            = 106,  /* temperatures, degC */
  VID_TEMP_TANK_3_C            = 107,  /* temperatures, degC */
  VID_TEMP_TANK_4_C            = 108,  /* temperatures, degC */
  VID_TEMP_TANK_5_C            = 109,  /* temperatures, degC */
  VID_TEMP_TANK_6_C            = 110,  /* temperatures, degC */
  VID_TEMP_WALL_IN_C           = 111,  /* temperatures, degC */
  VID_TEMP_WALL_OUT_C          = 112,  /* temperatures, degC */
  VID_TEMP_BOILER_EXHAUST_C    = 113,  /* temperatures, degC */
  VID_TEMP_RAD_H_C             = 114,  /* temperatures, degC */
  VID_TEMP_RAD_C_C             = 115,  /* temperatures, degC */
  VID_TEMP_TANK_IN_C_C         = 116,  /* temperatures, degC */
  VID_TEMP_TANK_OUT_C_C        = 117,  /* temperatures, degC */
  VID_TEMP_TECHM_BOARD_C       = 120,  /* temperatures, degC */
  VID_TEMP_IOBOARD_D_C         = 121,  /* temperatures, degC */
  VID_TEMP_IOBOARD_U_C         = 122,  /* temperatures, degC */
  VID_TEMP_ELECON_BOARD_C      = 123,  /* temperatures, degC */
  VID_TEMP_DOWNSTAIRS_C        = 124,  /* temperatures, degC */
  VID_TEMP_OFFICE_C            = 125,  /* temperatures, degC */
  VID_TEMP_KIDROOM_C           = 126,  /* temperatures, degC */
  VID_TEMP_OUTSIDE_C           = 127,  /* temperatures, degC */
  VID_TEMP_RECU_FC_C           = 128,  /* recu, degC */
  VID_TEMP_RECU_FH_C           = 129,  /* recu, degC */
  VID_TEMP_RECU_WH_C           = 130,  /* recu, degC */
  VID_TEMP_RECU_WC_C           = 131,  /* recu, degC */
  VID_RH_RECU_FH               = 132,  /* recu, % */
  VID_RH_RECU_WASTE            = 133,  /* recu, % */
  VID_CO2_RECU                 = 137,  /* recu, ppm */
  VID_DP_RECU_F                = 138,  /* recu, Pa */
  VID_DP_RECU_WASTE            = 139,  /* recu, Pa */
  VID_RECU_FAN_F               = 140,  /* recu, % */
  VID_RECU_FAN_WASTE           = 141,  /* recu, % */
  VID_TEMP_DILNA_C             = 144,  /* temperatures, degC */
  VID_TEMP_AKUPACK1_C          = 145,  /* temperatures, degC */
  VID_TEMP_BOARD_ELECON_D_C    = 146,  /* temperatures, degC */
  VID_TEMP_KOLARNA_C           = 147,  /* temperatures, degC */
  VID_METEO_WIND_BURST         = 161,  /* meteo, m/s */
  VID_METEO_WIND_AVG           = 162,  /* meteo, m/s */
  VID_METEO_WIND_POW           = 163,  /* meteo, W */
  VID_METEO_WIND_ENERGY        = 164,  /* meteo, Wh */
  VID_BMS1_CELL_V_1            = 180,  /* bms1, V */
  VID_BMS1_CELL_V_2            = 181,  /* bms1, V */
  VID_BMS1_CELL_V_3            = 182,  /* bms1, V */
  VID_BMS1_CELL_V_4            = 183,  /* bms1, V */
  VID_BMS1_CELL_V_5            = 184,  /* bms1, V */
  VID_BMS1_CELL_V_6            = 185,  /* bms1, V */
  VID_BMS1_CELL_V_7            = 186,  /* bms1, V */
  VID_BMS1_CELL_V_8            = 187,  /* bms1, V */
  VID_BMS1_CELL_V_9            = 188,  /* bms1, V */
  VID_BMS1_CELL_V_10           = 189,  /* bms1, V */
  VID_BMS1_CELL_V_11           = 190,  /* bms1, V */
  VID_BMS1_CELL_V_12           = 191,  /* bms1, V */
  VID_BMS1_CELL_V_13           = 192,  /* bms1, V */
  VID_BMS1_CELL_V_14           = 193,  /* bms1, V */
  VID_BMS1_CELL_V_15           = 194,  /* bms1, V */
  VID_BMS1_CELL_V_16           = 195,  /* bms1, V */
  VID_BMS1_CELL_TEMP_C_1       = 196,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_2       = 197,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_3       = 198,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_4       = 199,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_5       = 200,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_6       = 201,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_7       = 202,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_8       = 203,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_9       = 204,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_10      = 205,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_11      = 206,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_12      = 207,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_13      = 208,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_14      = 209,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_15      = 210,  /* bms1, degC */
  VID_BMS1_CELL_TEMP_C_16      = 211,  /* bms1, degC */
  VID_BMS2_CELL_V_1            = 220,  /* bms2, V */
  VID_BMS2_CELL_V_2            = 221,  /* bms2, V */
  VID_BMS2_CELL_V_3            = 222,  /* bms2, V */
  VID_BMS2_CELL_V_4            = 223,  /* bms2, V */
  VID_BMS2_CELL_V_5            = 224,  /* bms2, V */
  VID_BMS2_CELL_V_6            = 225,  /* bms2, V */
  VID_BMS2_CELL_V_7            = 226,  /* bms2, V */
  VID_BMS2_CELL_V_8            = 227,  /* bms2, V */
  VID_BMS2_CELL_V_9            = 228,  /* bms2, V */
  VID_BMS2_CELL_V_10           = 229,  /* bms2, V */
  VID_BMS2_CELL_V_11           = 230,  /* bms2, V */
  VID_BMS2_CELL_V_12           = 231,  /* bms2, V */
  VID_BMS2_CELL_V_13           = 232,  /* bms2, V */
  VID_BMS2_CELL_V_14           = 233,  /* bms2, V */
  VID_BMS2_CELL_V_15           = 234,  /* bms2, V */
  VID_BMS2_CELL_V_16           = 235,  /* bms2, V */
  VID_BMS2_CELL_TEMP_C_1       = 236,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_2       = 237,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_3       = 238,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_4       = 239,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_5       = 240,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_6       = 241,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_7       = 242,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_8       = 243,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_9       = 244,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_10      = 245,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_11      = 246,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_12      = 247,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_13      = 248,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_14      = 249,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_15      = 250,  /* bms2, degC */
  VID_BMS2_CELL_TEMP_C_16      = 251,  /* bms2, degC */
  VID_CONS_AXPERT_WH           = 260,  /* el_meters, Wh */
  VID_CONS_EVSE_WH             = 261,  /* el_meters, Wh */
  VID_CONS_WS_HEATING_WH       = 262,  /* el_meters, Wh */
  VID_POW_AXPERT_W             = 270,  /* el_meters, W */
  VID_POW_EVSE_W               = 271,  /* el_meters, W */
  VID_POW_WS_HEATING_W         = 272,  /* el_meters, W */
  VID_AXPERT_TEMP_C            = 278,  /* axpert, degC */
  VID_AXPERT_AC_POWER_W        = 279,  /* axpert, W */
  VID_AXPERT_BAT_CHARGING_A    = 280,  /* axpert, A */
  VID_AXPERT_BAT_DISCHARGING_A = 281,  /* axpert, A */
  VID_AXPERT_BAT_VOLTAGE_V     = 282,  /* axpert, V */
  VID_AXPERT_PVS1_W            = 283,  /* axpert, W */
  VID_AXPERT_PVS1_V            = 284,  /* axpert, V */
  VID_AXPERT_PVS1_A            = 285,  /* axpert, A */
  VID_AXPERT_PVS2_W            = 286,  /* axpert, W */
  VID_AXPERT_PVS2_V            = 287,  /* axpert, V */
  VID_AXPERT_PVS2_A            = 288,  /* axpert, A */
  VID_AXPERT_BAT_POWER_W       = 290,  /* axpert, W */
  VID_AXPERT_BAT_CURRENT_A     = 291,  /* axpert, A */
  VID_AXPERT_SOLAR_W           = 292,  /* axpert, W */
  VID_AXPERT_ENERGY_TODAY_WH   = 293,  /* axpert, Wh */
  VID_AXPERT_LOAD_W            = 294,  /* axpert, W */
  VID_STRG1_SOC                = 300,  /* storage1, % */
  VID_STRG1_POWER_W            = 301,  /* storage1, W */
  VID_STRG1_CURRENT_A          = 302,  /* storage1, A */
  VID_STRG1_ENERGY_WH          = 303,  /* storage1, Wh */
  VID_STRG1_OPT_CHARGING_A     = 304,  /* storage1, A */
  VID_STRG1_CAPACITY_AH        = 305,  /* storage1, Ah */
  VID_STRG1_BALANCED_TODAY     = 306,  /* storage1 */
  VID_STRG1_VOLTAGE_V          = 307,  /* storage1, V */
  VID_LLOAD1_POWER_W           = 308,  /* storage1, W */
  VID_LLOAD1_CONS_WH           = 309,  /* storage1, Wh */
  VID_LPV1_POWER_W             = 310,  /* storage1, W */
  VID_LPV1_PROD_WH             = 311,  /* storage1, Wh */
  VID_STRG2_SOC                = 320,  /* storage2, % */
  VID_STRG2_POWER_W            = 321,  /* storage2, W */
  VID_STRG2_CURRENT_A          = 322,  /* storage2, A */
  VID_STRG2_ENERGY_WH          = 323,  /* storage2, Wh */
  VID_STRG2_OPT_CHARGING_A     = 324,  /* storage2, A */
  VID_STRG2_CAPACITY_AH        = 325,  /* storage2, Ah */
  VID_STRG2_BALANCED_TODAY     = 326,  /* storage2 */
  VID_STRG2_VOLTAGE_V          = 327,  /* storage2, V */
  VID_LLOAD2_POWER_W           = 328,  /* storage2, W */
  VID_LLOAD2_CONS_WH           = 329,  /* storage2, Wh */
  VID_LPV2_POWER_W             = 330,  /* storage2, W */
  VID_LPV2_PROD_WH             = 331,  /* storage2, Wh */
  VID_BMS3_CELL_V_1            = 350,  /* bms3, V */
  VID_BMS3_CELL_V_2            = 351,  /* bms3, V */
  VID_BMS3_CELL_V_3            = 352,  /* bms3, V */
  VID_BMS3_CELL_V_4            = 353,  /* bms3, V */
  VID_BMS3_CELL_V_5            = 354,  /* bms3, V */
  VID_BMS3_CELL_V_6            = 355,  /* bms3, V */
  VID_BMS3_CELL_V_7            = 356,  /* bms3, V */
  VID_BMS3_CELL_V_8            = 357,  /* bms3, V */
  VID_BMS3_CELL_V_9            = 358,  /* bms3, V */
  VID_BMS3_CELL_V_10           = 359,  /* bms3, V */
  VID_BMS3_CELL_V_11           = 360,  /* bms3, V */
  VID_BMS3_CELL_V_12           = 361,  /* bms3, V */
  VID_BMS3_CELL_V_13           = 362,  /* bms3, V */
  VID_BMS3_CELL_V_14           = 363,  /* bms3, V */
  VID_BMS3_CELL_V_15           = 364,  /* bms3, V */
  VID_BMS3_CELL_V_16           = 365,  /* bms3, V */
  VID_BMS3_CELL_TEMP_C_1       = 366,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_2       = 367,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_3       = 368,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_4       = 369,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_5       = 370,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_6       = 371,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_7       = 372,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_8       = 373,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_9       = 374,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_10      = 375,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_11      = 376,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_12      = 377,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_13      = 378,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_14      = 379,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_15      = 380,  /* bms3, degC */
  VID_BMS3_CELL_TEMP_C_16      = 381,  /* bms3, degC */
} eVarId;

/* ---- enums ---- */
typedef enum
{
  ElHeaterState_Heating              = 0,
  ElHeaterState_TempReached          = 1,
  ElHeaterState_NoFreePower          = 2,
  ElHeaterState_LowSOC               = 3,
  ElHeaterState_48VFail              = 4,
  ElHeaterState_TankOvertemperature  = 5,
  ElHeaterState_BoardOvertemperature = 6,
  ElHeaterState_InvalidInputs        = 7,
  ElHeaterState_EleconComError       = 8,
  ElHeaterState_SwitchError          = 9,
  ElHeaterState_BalanceSupport       = 10,
  ElHeaterState_ComfortHeat          = 11,
} eElHeaterState;

/* ---- group structs (4-byte fields only) ---- */
typedef struct
{
  float SolarPower_W;        /* id 8, W, not on CAN */
  float SolarEnergyToday_Wh; /* id 9, Wh, not on CAN */
  float BatSoc;              /* id 10, % */
  float BatVoltage_V;        /* id 11, V */
  float Load_A;              /* id 12, A */
  float Charging_A;          /* id 13, A */
  float BatCurrent_A;        /* id 14, A */
  float ConsToday_Wh;        /* id 15, Wh, not on CAN */
  float BatEnergy_Wh;        /* id 16, Wh, not on CAN */
  float Load_W;              /* id 17, W, not on CAN */
  float BatPower_W;          /* id 18, W, not on CAN */
} sVarsGlobalEnergy;

typedef struct
{
  float Soc;             /* id 20, % */
  float Current_A;       /* id 21, A */
  float Voltage_V;       /* id 22, V */
  float EnergyStored_Wh; /* id 23, Wh */
  float TodayEnergy_Wh;  /* id 24, Wh */
  float Cell_V[16];      /* id 180..195, V */
  float CellTemp_C[16];  /* id 196..211, degC */
} sVarsBms1;

typedef struct
{
  float Soc;             /* id 30, %, not on CAN */
  float Current_A;       /* id 31, A, not on CAN */
  float Voltage_V;       /* id 32, V, not on CAN */
  float EnergyStored_Wh; /* id 33, Wh, not on CAN */
  float TodayEnergy_Wh;  /* id 34, Wh, not on CAN */
  float Cell_V[16];      /* id 220..235, V, not on CAN */
  float CellTemp_C[16];  /* id 236..251, degC, not on CAN */
} sVarsBms2;

typedef struct
{
  float Soc;             /* id 35, %, not on CAN */
  float Current_A;       /* id 36, A, not on CAN */
  float Voltage_V;       /* id 37, V, not on CAN */
  float EnergyStored_Wh; /* id 38, Wh, not on CAN */
  float TodayEnergy_Wh;  /* id 39, Wh, not on CAN */
  float Cell_V[16];      /* id 350..365, V, not on CAN */
  float CellTemp_C[16];  /* id 366..381, degC, not on CAN */
} sVarsBms3;

typedef struct
{
  float BatCurrent_A;      /* id 40, A, not on CAN */
  float BatVoltage_V;      /* id 41, V, not on CAN */
  float YieldToday_Wh;     /* id 42, Wh, not on CAN */
  float MaxToday_W;        /* id 43, W, not on CAN */
  float SolarPower_W;      /* id 44, W, not on CAN */
  float SolarVoltage_V;    /* id 45, V */
  float SolarCurrent_A;    /* id 46, A, not on CAN */
  float SolarMaxVoltage_V; /* id 47, V, not on CAN */
  float MaxBatCurrent_A;   /* id 48, A, not on CAN */
  float Temp_C;            /* id 49, degC, not on CAN */
} sVarsMppt;

typedef struct
{
  float Pck1Current_A; /* id 50, A */
  float Pck2Current_A; /* id 51, A, not on CAN */
} sVarsShunt;

typedef struct
{
  float    Strg1Soc;           /* id 300, %, not on CAN */
  float    Strg1Power_W;       /* id 301, W, not on CAN */
  float    Strg1Current_A;     /* id 302, A, not on CAN */
  float    Strg1Energy_Wh;     /* id 303, Wh, not on CAN */
  float    Strg1OptCharging_A; /* id 304, A, not on CAN */
  float    Strg1Capacity_Ah;   /* id 305, Ah, not on CAN */
  uint32_t Strg1BalancedToday; /* id 306, bool, not on CAN */
  float    Strg1Voltage_V;     /* id 307, V, not on CAN */
  float    Lload1Power_W;      /* id 308, W, not on CAN */
  float    Lload1Cons_Wh;      /* id 309, Wh, not on CAN */
  float    Lpv1Power_W;        /* id 310, W, not on CAN */
  float    Lpv1Prod_Wh;        /* id 311, Wh, not on CAN */
} sVarsStorage1;

typedef struct
{
  float    Strg2Soc;           /* id 320, % */
  float    Strg2Power_W;       /* id 321, W */
  float    Strg2Current_A;     /* id 322, A */
  float    Strg2Energy_Wh;     /* id 323, Wh */
  float    Strg2OptCharging_A; /* id 324, A */
  float    Strg2Capacity_Ah;   /* id 325, Ah */
  uint32_t Strg2BalancedToday; /* id 326, bool */
  float    Strg2Voltage_V;     /* id 327, V */
  float    Lload2Power_W;      /* id 328, W */
  float    Lload2Cons_Wh;      /* id 329, Wh */
  float    Lpv2Power_W;        /* id 330, W */
  float    Lpv2Prod_Wh;        /* id 331, Wh */
} sVarsStorage2;

typedef struct
{
  float Temp_C;           /* id 278, degC */
  float AcPower_W;        /* id 279, W */
  float BatCharging_A;    /* id 280, A */
  float BatDischarging_A; /* id 281, A */
  float BatVoltage_V;     /* id 282, V */
  float Pvs1_W;           /* id 283, W */
  float Pvs1_V;           /* id 284, V */
  float Pvs1_A;           /* id 285, A */
  float Pvs2_W;           /* id 286, W */
  float Pvs2_V;           /* id 287, V */
  float Pvs2_A;           /* id 288, A */
  float BatPower_W;       /* id 290, W */
  float BatCurrent_A;     /* id 291, A */
  float Solar_W;          /* id 292, W */
  float EnergyToday_Wh;   /* id 293, Wh */
  float Load_W;           /* id 294, W */
} sVarsAxpert;

typedef struct
{
  float ConsAc300_Wh;        /* id 60, Wh, not on CAN */
  float ConsAc3kw_Wh;        /* id 61, Wh, not on CAN */
  float ConsAc5kw_Wh;        /* id 62, Wh, not on CAN */
  float ConsFridge_Wh;       /* id 63, Wh */
  float ConsKitchen_Wh;      /* id 64, Wh */
  float ConsWaschmachine_Wh; /* id 65, Wh */
  float ConsOther_Wh;        /* id 66, Wh */
  float ConsTechm_Wh;        /* id 67, Wh */
  float ConsKitchenA_Wh;     /* id 68, Wh, not on CAN */
  float ConsKitchenB_Wh;     /* id 69, Wh, not on CAN */
  float PowAc300_W;          /* id 70, W, not on CAN */
  float PowAc3kw_W;          /* id 71, W, not on CAN */
  float PowAc5kw_W;          /* id 72, W, not on CAN */
  float PowFridge_W;         /* id 73, W */
  float PowKitchen_W;        /* id 74, W */
  float PowWaschmachine_W;   /* id 75, W */
  float PowOther_W;          /* id 76, W */
  float PowTechm_W;          /* id 77, W */
  float PowKitchenA_W;       /* id 78, W, not on CAN */
  float PowKitchenB_W;       /* id 79, W, not on CAN */
  float ConsAxpert_Wh;       /* id 260, Wh */
  float ConsEvse_Wh;         /* id 261, Wh */
  float ConsWsHeating_Wh;    /* id 262, Wh */
  float PowAxpert_W;         /* id 270, W */
  float PowEvse_W;           /* id 271, W */
  float PowWsHeating_W;      /* id 272, W */
} sVarsElMeters;

typedef struct
{
  uint32_t Status;    /* id 80, enum eElHeaterState */
  float    Load_A;    /* id 81, A */
  float    Current_A; /* id 82, A */
  float    Cons_Wh;   /* id 83, Wh */
} sVarsElHeater;

typedef struct
{
  float HeatTotal_Wh;   /* id 85, Wh */
  float HeatHeating_Wh; /* id 86, Wh */
  float BoilerPower_W;  /* id 98, W */
  float BoilerHeat;     /* id 99, Wh */
} sVarsHeating;

typedef struct
{
  float FlowCold_Lpm; /* id 90, l/min */
  float FlowHot_Lpm;  /* id 91, l/min */
  float ConsCold_L;   /* id 92, l */
  float ConsHot_L;    /* id 93, l */
} sVarsWater;

typedef struct
{
  float TempRecuFc_C; /* id 128, degC */
  float TempRecuFh_C; /* id 129, degC */
  float TempRecuWh_C; /* id 130, degC */
  float TempRecuWc_C; /* id 131, degC */
  float RhRecuFh;     /* id 132, % */
  float RhRecuWaste;  /* id 133, % */
  float Co2Recu;      /* id 137, ppm */
  float DpRecuF;      /* id 138, Pa */
  float DpRecuWaste;  /* id 139, Pa */
  float RecuFanF;     /* id 140, % */
  float RecuFanWaste; /* id 141, % */
} sVarsRecu;

typedef struct
{
  float Boiler_C;        /* id 100, degC */
  float BoilerIn_C;      /* id 101, degC */
  float BoilerOut_C;     /* id 102, degC */
  float TankInH_C;       /* id 103, degC */
  float TankOutH_C;      /* id 104, degC */
  float Tank1_C;         /* id 105, degC */
  float Tank2_C;         /* id 106, degC */
  float Tank3_C;         /* id 107, degC */
  float Tank4_C;         /* id 108, degC */
  float Tank5_C;         /* id 109, degC */
  float Tank6_C;         /* id 110, degC */
  float WallIn_C;        /* id 111, degC */
  float WallOut_C;       /* id 112, degC */
  float BoilerExhaust_C; /* id 113, degC */
  float RadH_C;          /* id 114, degC */
  float RadC_C;          /* id 115, degC */
  float TankInC_C;       /* id 116, degC */
  float TankOutC_C;      /* id 117, degC */
  float TechmBoard_C;    /* id 120, degC */
  float IoboardD_C;      /* id 121, degC */
  float IoboardU_C;      /* id 122, degC */
  float EleconBoard_C;   /* id 123, degC */
  float Downstairs_C;    /* id 124, degC */
  float Office_C;        /* id 125, degC */
  float Kidroom_C;       /* id 126, degC */
  float Outside_C;       /* id 127, degC */
  float Dilna_C;         /* id 144, degC */
  float Akupack1_C;      /* id 145, degC */
  float BoardEleconD_C;  /* id 146, degC */
  float Kolarna_C;       /* id 147, degC */
} sVarsTemperatures;

typedef struct
{
  float Burst;  /* id 161, m/s */
  float Avg;    /* id 162, m/s */
  float Pow;    /* id 163, W */
  float Energy; /* id 164, Wh */
} sVarsMeteo;

typedef struct
{
  sVarsGlobalEnergy GlobalEnergy;
  sVarsBms1         Bms1;
  sVarsBms2         Bms2;
  sVarsBms3         Bms3;
  sVarsMppt         Mppt;
  sVarsShunt        Shunt;
  sVarsStorage1     Storage1;
  sVarsStorage2     Storage2;
  sVarsAxpert       Axpert;
  sVarsElMeters     ElMeters;
  sVarsElHeater     ElHeater;
  sVarsHeating      Heating;
  sVarsWater        Water;
  sVarsRecu         Recu;
  sVarsTemperatures Temperatures;
  sVarsMeteo        Meteo;
} sVars;

extern sVars Vars2;
extern const sVarMeta VarMeta[];        /* VARS_COUNT entries, sorted by id */
extern const uint16_t VarIdToIndex[];   /* VAR_MAX_ID + 1 entries, VAR_IDX_UNKNOWN = unknown id */

#endif /* UHA_VARS_GEN_H_ */
