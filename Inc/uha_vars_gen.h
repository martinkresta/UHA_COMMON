/* AUTO-GENERATED from uha_vars.yaml - DO NOT EDIT
 * YAML version: 0
 * regenerate with tools/gen_all.py
 */

#ifndef UHA_VARS_GEN_H_
#define UHA_VARS_GEN_H_

#include <stdint.h>
#include "VARS2_types.h"

#define VARS_COUNT         312
#define VAR_MAX_ID         513
#define VARS_ON_CAN_COUNT  199
#define VARS_ON_CHANGE_COUNT 50

/* ---- variable ids ---- */
typedef enum
{
  VID_NETWORK_STATUS             =   1,  /* System */
  VID_SOLAR_POWER_W              =   8,  /* GlobalEnergy.Solar, W */
  VID_SOLAR_ENERGY_TODAY_WH      =   9,  /* GlobalEnergy.Solar, Wh */
  VID_BAT_SOC                    =  10,  /* GlobalEnergy.Battery, % */
  VID_BAT_VOLTAGE_V              =  11,  /* GlobalEnergy.Battery, V */
  VID_LOAD_A                     =  12,  /* GlobalEnergy.Load, A */
  VID_CHARGING_A                 =  13,  /* Mppt, A */
  VID_BAT_CURRENT_A              =  14,  /* GlobalEnergy.Battery, A */
  VID_CONS_TODAY_WH              =  15,  /* GlobalEnergy.Load, Wh */
  VID_BAT_ENERGY_WH              =  16,  /* GlobalEnergy.Battery, Wh */
  VID_LOAD_W                     =  17,  /* GlobalEnergy.Load, W */
  VID_BAT_POWER_W                =  18,  /* GlobalEnergy.Battery, W */
  VID_BMS1_SOC                   =  20,  /* PackWorkshop251Ah, % */
  VID_BMS1_CURRENT_A             =  21,  /* PackWorkshop251Ah, A */
  VID_BMS1_VOLTAGE_V             =  22,  /* PackWorkshop251Ah, V */
  VID_BMS1_ENERGY_STORED_WH      =  23,  /* PackWorkshop251Ah, Wh */
  VID_BMS1_TODAY_ENERGY_WH       =  24,  /* PackWorkshop251Ah, Wh */
  VID_BMS2_SOC                   =  30,  /* PackHome220Ah, % */
  VID_BMS2_CURRENT_A             =  31,  /* PackHome220Ah, A */
  VID_BMS2_VOLTAGE_V             =  32,  /* PackHome220Ah, V */
  VID_BMS2_ENERGY_STORED_WH      =  33,  /* PackHome220Ah, Wh */
  VID_BMS2_TODAY_ENERGY_WH       =  34,  /* PackHome220Ah, Wh */
  VID_BMS3_SOC                   =  35,  /* PackHome80Ah, % */
  VID_BMS3_CURRENT_A             =  36,  /* PackHome80Ah, A */
  VID_BMS3_VOLTAGE_V             =  37,  /* PackHome80Ah, V */
  VID_BMS3_ENERGY_STORED_WH      =  38,  /* PackHome80Ah, Wh */
  VID_BMS3_TODAY_ENERGY_WH       =  39,  /* PackHome80Ah, Wh */
  VID_MPPT_BAT_CURRENT_A         =  40,  /* Mppt, A */
  VID_MPPT_BAT_VOLTAGE_V         =  41,  /* Mppt, V */
  VID_MPPT_YIELD_TODAY_WH        =  42,  /* Mppt, Wh */
  VID_MPPT_MAX_TODAY_W           =  43,  /* Mppt, W */
  VID_MPPT_SOLAR_POWER_W         =  44,  /* Mppt, W */
  VID_MPPT_SOLAR_VOLTAGE_V       =  45,  /* Mppt, V */
  VID_MPPT_SOLAR_CURRENT_A       =  46,  /* Mppt, A */
  VID_MPPT_SOLAR_MAX_VOLTAGE_V   =  47,  /* Mppt, V */
  VID_MPPT_MAX_BAT_CURRENT_A     =  48,  /* Mppt, A */
  VID_MPPT_TEMP_C                =  49,  /* Mppt, degC */
  VID_SHUNT_PCK1_CURRENT_A       =  50,  /* ShuntWorkshop, A */
  VID_SHUNT_PCK2_CURRENT_A       =  51,  /* ShuntHome, A */
  VID_CONS_AC300_WH              =  60,  /* Meters.Elecon, Wh */
  VID_CONS_AC3KW_WH              =  61,  /* Meters.Elecon, Wh */
  VID_CONS_AC5KW_WH              =  62,  /* Meters.Elecon, Wh */
  VID_CONS_FRIDGE_WH             =  63,  /* Meters.IoboardD, Wh */
  VID_CONS_KITCHEN_WH            =  64,  /* Meters.IoboardD, Wh */
  VID_CONS_WASCHMACHINE_WH       =  65,  /* Meters.IoboardD, Wh */
  VID_CONS_OTHER_WH              =  66,  /* Meters.IoboardD, Wh */
  VID_CONS_TECHM_WH              =  67,  /* Meters.IoboardD, Wh */
  VID_CONS_KITCHEN_A_WH          =  68,  /* Meters.IoboardD, Wh */
  VID_CONS_KITCHEN_B_WH          =  69,  /* Meters.IoboardD, Wh */
  VID_POW_AC300_W                =  70,  /* Meters.Elecon, W */
  VID_POW_AC3KW_W                =  71,  /* Meters.Elecon, W */
  VID_POW_AC5KW_W                =  72,  /* Meters.Elecon, W */
  VID_POW_FRIDGE_W               =  73,  /* Meters.IoboardD, W */
  VID_POW_KITCHEN_W              =  74,  /* Meters.IoboardD, W */
  VID_POW_WASCHMACHINE_W         =  75,  /* Meters.IoboardD, W */
  VID_POW_OTHER_W                =  76,  /* Meters.IoboardD, W */
  VID_POW_TECHM_W                =  77,  /* Meters.IoboardD, W */
  VID_POW_KITCHEN_A_W            =  78,  /* Meters.IoboardD, W */
  VID_POW_KITCHEN_B_W            =  79,  /* Meters.IoboardD, W */
  VID_EL_HEATER_STATUS           =  80,  /* ElHeater */
  VID_EL_HEATER_LOAD_A           =  81,  /* ElHeater, A */
  VID_EL_HEATER_CURRENT_A        =  82,  /* ElHeater, A */
  VID_EL_HEATER_CONS_WH          =  83,  /* ElHeater, Wh */
  VID_HEAT_TOTAL_WH              =  85,  /* Tank, Wh */
  VID_HEAT_HEATING_WH            =  86,  /* Tank, Wh */
  VID_FLOW_COLD_LPM              =  90,  /* Water, l/min */
  VID_FLOW_HOT_LPM               =  91,  /* Water, l/min */
  VID_CONS_COLD_L                =  92,  /* Water, l */
  VID_CONS_HOT_L                 =  93,  /* Water, l */
  VID_BOILER_POWER_W             =  98,  /* Boiler, W */
  VID_BOILER_HEAT                =  99,  /* Boiler, Wh */
  VID_TEMP_BOILER_C              = 100,  /* Temperatures.Techm, degC */
  VID_TEMP_BOILER_IN_C           = 101,  /* Temperatures.Techm, degC */
  VID_TEMP_BOILER_OUT_C          = 102,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_IN_H_C           = 103,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_OUT_H_C          = 104,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_1_C              = 105,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_2_C              = 106,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_3_C              = 107,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_4_C              = 108,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_5_C              = 109,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_6_C              = 110,  /* Temperatures.Techm, degC */
  VID_TEMP_WALL_IN_C             = 111,  /* Temperatures.Techm, degC */
  VID_TEMP_WALL_OUT_C            = 112,  /* Temperatures.Techm, degC */
  VID_TEMP_BOILER_EXHAUST_C      = 113,  /* Temperatures.Techm, degC */
  VID_TEMP_RAD_H_C               = 114,  /* Temperatures.Techm, degC */
  VID_TEMP_RAD_C_C               = 115,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_IN_C_C           = 116,  /* Temperatures.Techm, degC */
  VID_TEMP_TANK_OUT_C_C          = 117,  /* Temperatures.Techm, degC */
  VID_TEMP_TECHM_BOARD_C         = 120,  /* Temperatures.Techm, degC */
  VID_TEMP_IOBOARD_D_C           = 121,  /* Temperatures.IoboardD, degC */
  VID_TEMP_IOBOARD_U_C           = 122,  /* Temperatures.IoboardU, degC */
  VID_TEMP_ELECON_BOARD_C        = 123,  /* Temperatures.Elecon, degC */
  VID_TEMP_DOWNSTAIRS_C          = 124,  /* Temperatures.IoboardD, degC */
  VID_TEMP_OFFICE_C              = 125,  /* Temperatures.Elecon, degC */
  VID_TEMP_KIDROOM_C             = 126,  /* Temperatures.Elecon, degC */
  VID_TEMP_OUTSIDE_C             = 127,  /* Temperatures.Elecon, degC */
  VID_TEMP_RECU_FC_C             = 128,  /* Recu.Air, degC */
  VID_TEMP_RECU_FH_C             = 129,  /* Recu.Air, degC */
  VID_TEMP_RECU_WH_C             = 130,  /* Recu.Air, degC */
  VID_TEMP_RECU_WC_C             = 131,  /* Recu.Air, degC */
  VID_RH_RECU_FH                 = 132,  /* Recu.Air, % */
  VID_RH_RECU_WASTE              = 133,  /* Recu.Air, % */
  VID_CO2_RECU                   = 137,  /* Recu.Air, ppm */
  VID_DP_RECU_F                  = 138,  /* Recu.Air, Pa */
  VID_DP_RECU_WASTE              = 139,  /* Recu.Air, Pa */
  VID_RECU_FAN_F                 = 140,  /* Recu.Fans, % */
  VID_RECU_FAN_WASTE             = 141,  /* Recu.Fans, % */
  VID_TEMP_DILNA_C               = 144,  /* Temperatures.EleconD, degC */
  VID_TEMP_AKUPACK1_C            = 145,  /* Temperatures.EleconD, degC */
  VID_TEMP_BOARD_ELECON_D_C      = 146,  /* Temperatures.EleconD, degC */
  VID_TEMP_KOLARNA_C             = 147,  /* Temperatures.EleconD, degC */
  VID_METEO_WIND_BURST           = 161,  /* Meteo, m/s */
  VID_METEO_WIND_AVG             = 162,  /* Meteo, m/s */
  VID_METEO_WIND_POW             = 163,  /* Meteo, W */
  VID_METEO_WIND_ENERGY          = 164,  /* Meteo, Wh */
  VID_BMS1_CELL_V_1              = 180,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_2              = 181,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_3              = 182,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_4              = 183,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_5              = 184,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_6              = 185,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_7              = 186,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_8              = 187,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_9              = 188,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_10             = 189,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_11             = 190,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_12             = 191,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_13             = 192,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_14             = 193,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_15             = 194,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_V_16             = 195,  /* PackWorkshop251Ah, V */
  VID_BMS1_CELL_TEMP_C_1         = 196,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_2         = 197,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_3         = 198,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_4         = 199,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_5         = 200,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_6         = 201,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_7         = 202,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_8         = 203,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_9         = 204,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_10        = 205,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_11        = 206,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_12        = 207,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_13        = 208,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_14        = 209,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_15        = 210,  /* PackWorkshop251Ah, degC */
  VID_BMS1_CELL_TEMP_C_16        = 211,  /* PackWorkshop251Ah, degC */
  VID_BMS2_CELL_V_1              = 220,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_2              = 221,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_3              = 222,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_4              = 223,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_5              = 224,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_6              = 225,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_7              = 226,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_8              = 227,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_9              = 228,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_10             = 229,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_11             = 230,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_12             = 231,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_13             = 232,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_14             = 233,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_15             = 234,  /* PackHome220Ah, V */
  VID_BMS2_CELL_V_16             = 235,  /* PackHome220Ah, V */
  VID_BMS2_CELL_TEMP_C_1         = 236,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_2         = 237,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_3         = 238,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_4         = 239,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_5         = 240,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_6         = 241,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_7         = 242,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_8         = 243,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_9         = 244,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_10        = 245,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_11        = 246,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_12        = 247,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_13        = 248,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_14        = 249,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_15        = 250,  /* PackHome220Ah, degC */
  VID_BMS2_CELL_TEMP_C_16        = 251,  /* PackHome220Ah, degC */
  VID_CONS_AXPERT_WH             = 260,  /* Meters.EleconD, Wh */
  VID_CONS_EVSE_WH               = 261,  /* Meters.EleconD, Wh */
  VID_CONS_WS_HEATING_WH         = 262,  /* Meters.EleconD, Wh */
  VID_POW_AXPERT_W               = 270,  /* Meters.EleconD, W */
  VID_POW_EVSE_W                 = 271,  /* Meters.EleconD, W */
  VID_POW_WS_HEATING_W           = 272,  /* Meters.EleconD, W */
  VID_AXPERT_TEMP_C              = 278,  /* Axpert, degC */
  VID_AXPERT_AC_POWER_W          = 279,  /* Axpert, W */
  VID_AXPERT_BAT_CHARGING_A      = 280,  /* Axpert, A */
  VID_AXPERT_BAT_DISCHARGING_A   = 281,  /* Axpert, A */
  VID_AXPERT_BAT_VOLTAGE_V       = 282,  /* Axpert, V */
  VID_AXPERT_PVS1_W              = 283,  /* Axpert, W */
  VID_AXPERT_PVS1_V              = 284,  /* Axpert, V */
  VID_AXPERT_PVS1_A              = 285,  /* Axpert, A */
  VID_AXPERT_PVS2_W              = 286,  /* Axpert, W */
  VID_AXPERT_PVS2_V              = 287,  /* Axpert, V */
  VID_AXPERT_PVS2_A              = 288,  /* Axpert, A */
  VID_AXPERT_BAT_POWER_W         = 290,  /* Axpert, W */
  VID_AXPERT_BAT_CURRENT_A       = 291,  /* Axpert, A */
  VID_AXPERT_SOLAR_W             = 292,  /* Axpert, W */
  VID_AXPERT_ENERGY_TODAY_WH     = 293,  /* Axpert, Wh */
  VID_AXPERT_LOAD_W              = 294,  /* Axpert, W */
  VID_STRG1_SOC                  = 300,  /* HomeEnergy.Storage, % */
  VID_STRG1_POWER_W              = 301,  /* HomeEnergy.Storage, W */
  VID_STRG1_CURRENT_A            = 302,  /* HomeEnergy.Storage, A */
  VID_STRG1_ENERGY_WH            = 303,  /* HomeEnergy.Storage, Wh */
  VID_STRG1_OPT_CHARGING_A       = 304,  /* HomeEnergy.Storage, A */
  VID_STRG1_CAPACITY_AH          = 305,  /* HomeEnergy.Storage, Ah */
  VID_STRG1_BALANCED_TODAY       = 306,  /* HomeEnergy.Storage */
  VID_STRG1_VOLTAGE_V            = 307,  /* HomeEnergy.Storage, V */
  VID_LLOAD1_POWER_W             = 308,  /* HomeEnergy.Load, W */
  VID_LLOAD1_CONS_WH             = 309,  /* HomeEnergy.Load, Wh */
  VID_LPV1_POWER_W               = 310,  /* HomeEnergy.Pv, W */
  VID_LPV1_PROD_WH               = 311,  /* HomeEnergy.Pv, Wh */
  VID_STRG2_SOC                  = 320,  /* WorkshopEnergy.Storage, % */
  VID_STRG2_POWER_W              = 321,  /* WorkshopEnergy.Storage, W */
  VID_STRG2_CURRENT_A            = 322,  /* WorkshopEnergy.Storage, A */
  VID_STRG2_ENERGY_WH            = 323,  /* WorkshopEnergy.Storage, Wh */
  VID_STRG2_OPT_CHARGING_A       = 324,  /* WorkshopEnergy.Storage, A */
  VID_STRG2_CAPACITY_AH          = 325,  /* WorkshopEnergy.Storage, Ah */
  VID_STRG2_BALANCED_TODAY       = 326,  /* WorkshopEnergy.Storage */
  VID_STRG2_VOLTAGE_V            = 327,  /* WorkshopEnergy.Storage, V */
  VID_LLOAD2_POWER_W             = 328,  /* WorkshopEnergy.Load, W */
  VID_LLOAD2_CONS_WH             = 329,  /* WorkshopEnergy.Load, Wh */
  VID_LPV2_POWER_W               = 330,  /* WorkshopEnergy.Pv, W */
  VID_LPV2_PROD_WH               = 331,  /* WorkshopEnergy.Pv, Wh */
  VID_BMS3_CELL_V_1              = 350,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_2              = 351,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_3              = 352,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_4              = 353,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_5              = 354,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_6              = 355,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_7              = 356,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_8              = 357,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_9              = 358,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_10             = 359,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_11             = 360,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_12             = 361,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_13             = 362,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_14             = 363,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_15             = 364,  /* PackHome80Ah, V */
  VID_BMS3_CELL_V_16             = 365,  /* PackHome80Ah, V */
  VID_BMS3_CELL_TEMP_C_1         = 366,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_2         = 367,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_3         = 368,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_4         = 369,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_5         = 370,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_6         = 371,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_7         = 372,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_8         = 373,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_9         = 374,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_10        = 375,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_11        = 376,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_12        = 377,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_13        = 378,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_14        = 379,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_15        = 380,  /* PackHome80Ah, degC */
  VID_BMS3_CELL_TEMP_C_16        = 381,  /* PackHome80Ah, degC */
  VID_ST_LIGHT_LOFT_WEST         = 400,  /* States.IoboardU, % */
  VID_ST_LIGHT_LOFT_EAST_1       = 401,  /* States.IoboardU, % */
  VID_ST_LIGHT_LOFT_EAST_2       = 402,  /* States.IoboardU, % */
  VID_ST_LIGHT_BEDROOM_SOUTH     = 403,  /* States.IoboardU, % */
  VID_ST_LIGHT_BEDROOM_NORTH     = 404,  /* States.IoboardU, % */
  VID_ST_LIGHT_BATHROOM          = 405,  /* States.IoboardU, % */
  VID_ST_LIGHT_STAIR_STEPS       = 406,  /* States.IoboardU, % */
  VID_ST_LIGHT_STAIRS            = 407,  /* States.IoboardU, % */
  VID_ST_LIGHT_HALLWAY           = 408,  /* States.IoboardU, % */
  VID_ST_LIGHT_LIVING_ROOM       = 409,  /* States.IoboardU, % */
  VID_ST_LIGHT_DINING_ROOM       = 410,  /* States.IoboardU, % */
  VID_ST_LIGHT_KITCHEN           = 411,  /* States.IoboardU, % */
  VID_ST_SCREEN_EAST             = 412,  /* States.IoboardU */
  VID_ST_SCREEN_WEST             = 413,  /* States.IoboardU */
  VID_ST_LIGHT_WASHBASIN         = 420,  /* States.IoboardD, % */
  VID_ST_LIGHT_KITCHEN_COUNTER   = 421,  /* States.IoboardD, % */
  VID_ST_SEWAGE_PUMP             = 422,  /* States.IoboardD */
  VID_ST_UP_REQ_DAY_C            = 440,  /* States.Techm, degC */
  VID_ST_UP_REQ_NIGHT_C          = 441,  /* States.Techm, degC */
  VID_ST_DOWN_REQ_DAY_C          = 442,  /* States.Techm, degC */
  VID_ST_DOWN_REQ_NIGHT_C        = 443,  /* States.Techm, degC */
  VID_ST_EXHAUST_SET_C           = 444,  /* States.Techm, degC */
  VID_ST_UP_REQ_C                = 445,  /* States.Techm, degC */
  VID_ST_DOWN_REQ_C              = 446,  /* States.Techm, degC */
  VID_ST_RAD_ON                  = 447,  /* States.Techm */
  VID_ST_WALL_ON                 = 448,  /* States.Techm */
  VID_ST_PUMP_BOILER             = 449,  /* States.Techm */
  VID_ST_PUMP_WALL               = 450,  /* States.Techm */
  VID_ST_PUMP_RAD                = 451,  /* States.Techm */
  VID_ST_EL_HEATER_COILS         = 452,  /* States.Techm */
  VID_ST_BOILER_STATE            = 453,  /* States.Techm */
  VID_TECHM_VALVE_WALL_PCT       = 460,  /* States.Techm, % */
  VID_TECHM_VALVE_RAD_PCT        = 461,  /* States.Techm, % */
  VID_TECHM_AIR_VALVE_SIGNAL_PCT = 462,  /* States.Techm, % */
  VID_TECHM_REQ_WATER_RAD_C      = 463,  /* States.Techm, degC */
  VID_TECHM_REQ_WATER_WALL_C     = 464,  /* States.Techm, degC */
  VID_ST_AC_300W                 = 480,  /* States.Elecon */
  VID_ST_AC_3KW                  = 481,  /* States.Elecon */
  VID_ST_AC_5KW                  = 482,  /* States.Elecon */
  VID_ST_LIGHT_EXTERIOR_HOUSE    = 483,  /* States.Elecon, % */
  VID_ST_AC_AXPERT               = 490,  /* States.EleconD */
  VID_ST_AC_VICTRON              = 491,  /* States.EleconD */
  VID_ST_AKUPACK_HEATER          = 492,  /* States.EleconD */
  VID_ST_AKUPACK_SET_DAY_C       = 493,  /* States.EleconD, degC */
  VID_ST_AKUPACK_SET_NIGHT_C     = 494,  /* States.EleconD, degC */
  VID_ST_EVSE_RELAY              = 495,  /* States.EleconD */
  VID_ST_WS_HEATING_RELAY        = 496,  /* States.EleconD */
  VID_ST_LIGHT_EXTERIOR_WS       = 497,  /* States.EleconD, % */
  VID_ST_LIGHT_KOLARNA           = 498,  /* States.EleconD, % */
  VID_ST_LIGHT_SKLEP             = 499,  /* States.EleconD, % */
  VID_ST_RECU_REMOTE_MODE        = 510,  /* States.Recu */
  VID_ST_RECU_LIMIT              = 511,  /* States.Recu */
  VID_ST_RECU_CO2_SETPOINT       = 512,  /* States.Recu, ppm */
  VID_ST_RECU_ANTIDRY_RH         = 513,  /* States.Recu, % */
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

typedef enum
{
  BlindCmd_None = 0,
  BlindCmd_Up   = 1,
  BlindCmd_Down = 2,
  BlindCmd_Stop = 3,
} eBlindCmd;

typedef enum
{
  BoilerStatus_Idle          = 0,
  BoilerStatus_HeatUp        = 1,
  BoilerStatus_Heating       = 2,
  BoilerStatus_CoolDown      = 3,
  BoilerStatus_Overheat      = 4,
  BoilerStatus_Error         = 5,
  BoilerStatus_InvalidInputs = 6,
} eBoilerStatus;

typedef enum
{
  RecuRemoteMode_Auto                = 0,
  RecuRemoteMode_SlightOverpressure  = 1,
  RecuRemoteMode_MaxOverpressure     = 2,
  RecuRemoteMode_SlightUnderpressure = 3,
  RecuRemoteMode_MaxUnderpressure    = 4,
} eRecuRemoteMode;

typedef enum
{
  RecuLimit_None              = 0,
  RecuLimit_Eco               = 1,
  RecuLimit_AntiDry           = 2,
  RecuLimit_AntifreezeReduced = 3,
  RecuLimit_AntifreezeStop    = 4,
  RecuLimit_BatteryOff        = 5,
  RecuLimit_InvalidInputs     = 6,
} eRecuLimit;

typedef enum
{
  InverterState_Off           = 0,
  InverterState_On            = 1,
  InverterState_OnKeepSetting = 2,
  InverterState_KeepOn        = 3,
  InverterState_AutoOff       = 4,
} eInverterState;

/* ---- group structs ({value, valid} fields, see VARS2_types.h) ---- */
typedef struct
{
  sVarU32 NetworkStatus; /* id 1, not on CAN */
} sVarsSystem;

typedef struct
{
  sVarF32 Soc_pct;   /* id 10, % */
  sVarF32 Voltage_V; /* id 11, V */
  sVarF32 Current_A; /* id 14, A */
  sVarF32 Energy_Wh; /* id 16, Wh, not on CAN */
  sVarF32 Power_W;   /* id 18, W, not on CAN */
} sVarsGlobalEnergyBattery;

typedef struct
{
  sVarF32 Power_W;        /* id 8, W, not on CAN */
  sVarF32 EnergyToday_Wh; /* id 9, Wh, not on CAN */
} sVarsGlobalEnergySolar;

typedef struct
{
  sVarF32 Current_A;    /* id 12, A */
  sVarF32 ConsToday_Wh; /* id 15, Wh, not on CAN */
  sVarF32 Power_W;      /* id 17, W, not on CAN */
} sVarsGlobalEnergyLoad;

typedef struct
{
  sVarsGlobalEnergyBattery Battery;  /* ELECON */
  sVarsGlobalEnergySolar   Solar;    /* ELECON */
  sVarsGlobalEnergyLoad    Load;     /* ELECON */
} sVarsGlobalEnergy;

typedef struct
{
  sVarF32 Soc_pct;           /* % */
  sVarF32 Current_A;         /* A */
  sVarF32 Voltage_V;         /* V */
  sVarF32 EnergyStored_Wh;   /* Wh */
  sVarF32 ChargedToday_Wh;   /* Wh */
  sVarF32 CellVoltage_V[16]; /* V */
  sVarF32 CellTemp_C[16];    /* degC */
} sVarsPack;

typedef struct
{
  sVarF32 BatCurrent_A;      /* id 40, A, not on CAN */
  sVarF32 BatVoltage_V;      /* id 41, V, not on CAN */
  sVarF32 YieldToday_Wh;     /* id 42, Wh, not on CAN */
  sVarF32 MaxPowerToday_W;   /* id 43, W, not on CAN */
  sVarF32 SolarPower_W;      /* id 44, W, not on CAN */
  sVarF32 SolarVoltage_V;    /* id 45, V */
  sVarF32 SolarCurrent_A;    /* id 46, A, not on CAN */
  sVarF32 SolarMaxVoltage_V; /* id 47, V, not on CAN */
  sVarF32 MaxBatCurrent_A;   /* id 48, A, not on CAN */
  sVarF32 Temp_C;            /* id 49, degC, not on CAN */
  sVarF32 Charging_A;        /* id 13, A */
} sVarsMppt;

typedef struct
{
  sVarF32 Current_A; /* A */
} sVarsShunt;

typedef struct
{
  sVarF32 Soc_pct;       /* % */
  sVarF32 Power_W;       /* W */
  sVarF32 Current_A;     /* A */
  sVarF32 Energy_Wh;     /* Wh */
  sVarF32 OptCharging_A; /* A */
  sVarF32 Capacity_Ah;   /* Ah */
  sVarI32 BalancedToday; /* bool */
  sVarF32 Voltage_V;     /* V */
} sVarsLocalEnergyStorage;

typedef struct
{
  sVarF32 Power_W; /* W */
  sVarF32 Cons_Wh; /* Wh */
} sVarsLocalEnergyLoad;

typedef struct
{
  sVarF32 Power_W; /* W */
  sVarF32 Prod_Wh; /* Wh */
} sVarsLocalEnergyPv;

typedef struct
{
  sVarsLocalEnergyStorage Storage;
  sVarsLocalEnergyLoad    Load;
  sVarsLocalEnergyPv      Pv;
} sVarsLocalEnergy;

typedef struct
{
  sVarF32 Temp_C;           /* id 278, degC */
  sVarF32 AcPower_W;        /* id 279, W */
  sVarF32 BatCharging_A;    /* id 280, A */
  sVarF32 BatDischarging_A; /* id 281, A */
  sVarF32 BatVoltage_V;     /* id 282, V */
  sVarF32 Pv1Power_W;       /* id 283, W */
  sVarF32 Pv1Voltage_V;     /* id 284, V */
  sVarF32 Pv1Current_A;     /* id 285, A */
  sVarF32 Pv2Power_W;       /* id 286, W */
  sVarF32 Pv2Voltage_V;     /* id 287, V */
  sVarF32 Pv2Current_A;     /* id 288, A */
  sVarF32 BatPower_W;       /* id 290, W */
  sVarF32 BatCurrent_A;     /* id 291, A */
  sVarF32 SolarPower_W;     /* id 292, W */
  sVarF32 EnergyToday_Wh;   /* id 293, Wh */
  sVarF32 LoadPower_W;      /* id 294, W */
} sVarsAxpert;

typedef struct
{
  sVarF32 Ac300Cons_Wh; /* id 60, Wh, not on CAN */
  sVarF32 Ac3kwCons_Wh; /* id 61, Wh, not on CAN */
  sVarF32 Ac5kwCons_Wh; /* id 62, Wh, not on CAN */
  sVarF32 Ac300Power_W; /* id 70, W, not on CAN */
  sVarF32 Ac3kwPower_W; /* id 71, W, not on CAN */
  sVarF32 Ac5kwPower_W; /* id 72, W, not on CAN */
} sVarsMetersElecon;

typedef struct
{
  sVarF32 FridgeCons_Wh;         /* id 63, Wh */
  sVarF32 KitchenCons_Wh;        /* id 64, Wh */
  sVarF32 WashingMachineCons_Wh; /* id 65, Wh */
  sVarF32 OtherCons_Wh;          /* id 66, Wh */
  sVarF32 TechmCons_Wh;          /* id 67, Wh */
  sVarF32 KitchenACons_Wh;       /* id 68, Wh, not on CAN */
  sVarF32 KitchenBCons_Wh;       /* id 69, Wh, not on CAN */
  sVarF32 FridgePower_W;         /* id 73, W */
  sVarF32 KitchenPower_W;        /* id 74, W */
  sVarF32 WashingMachinePower_W; /* id 75, W */
  sVarF32 OtherPower_W;          /* id 76, W */
  sVarF32 TechmPower_W;          /* id 77, W */
  sVarF32 KitchenAPower_W;       /* id 78, W, not on CAN */
  sVarF32 KitchenBPower_W;       /* id 79, W, not on CAN */
} sVarsMetersIoboardD;

typedef struct
{
  sVarF32 AxpertCons_Wh;  /* id 260, Wh */
  sVarF32 EvseCons_Wh;    /* id 261, Wh */
  sVarF32 HeatingCons_Wh; /* id 262, Wh */
  sVarF32 AxpertPower_W;  /* id 270, W */
  sVarF32 EvsePower_W;    /* id 271, W */
  sVarF32 HeatingPower_W; /* id 272, W */
} sVarsMetersEleconD;

typedef struct
{
  sVarsMetersElecon   Elecon;    /* ELECON, validity per field */
  sVarsMetersIoboardD IoboardD;  /* IOBOARD_D, validity per field */
  sVarsMetersEleconD  EleconD;   /* ELECON_D, validity per field */
} sVarsMeters;

typedef struct
{
  sVarI32 State;        /* id 80, enum eElHeaterState */
  sVarF32 Load_A;       /* id 81, A */
  sVarF32 Current_A;    /* id 82, A */
  sVarF32 ConsToday_Wh; /* id 83, Wh */
} sVarsElHeater;

typedef struct
{
  sVarF32 TotalEnergy_Wh;     /* id 85, Wh */
  sVarF32 AvailableEnergy_Wh; /* id 86, Wh */
} sVarsTank;

typedef struct
{
  sVarF32 Power_W;      /* id 98, W */
  sVarF32 HeatToday_Wh; /* id 99, Wh */
} sVarsBoiler;

typedef struct
{
  sVarF32 ColdFlow_lpm; /* id 90, l/min */
  sVarF32 HotFlow_lpm;  /* id 91, l/min */
  sVarF32 ColdCons_l;   /* id 92, l */
  sVarF32 HotCons_l;    /* id 93, l */
} sVarsWater;

typedef struct
{
  sVarF32 FreshCold_C;    /* id 128, degC */
  sVarF32 FreshHot_C;     /* id 129, degC */
  sVarF32 WasteHot_C;     /* id 130, degC */
  sVarF32 WasteCold_C;    /* id 131, degC */
  sVarF32 FreshHotRh_pct; /* id 132, % */
  sVarF32 WasteHotRh_pct; /* id 133, % */
  sVarF32 Co2_ppm;        /* id 137, ppm */
  sVarF32 FreshDp_Pa;     /* id 138, Pa */
  sVarF32 WasteDp_Pa;     /* id 139, Pa */
} sVarsRecuAir;

typedef struct
{
  sVarF32 Fresh_pct; /* id 140, % */
  sVarF32 Waste_pct; /* id 141, % */
} sVarsRecuFans;

typedef struct
{
  sVarsRecuAir  Air;   /* RECU, validity per field */
  sVarsRecuFans Fans;  /* RECU */
} sVarsRecu;

typedef struct
{
  sVarF32 Boiler_C;        /* id 100, degC */
  sVarF32 BoilerIn_C;      /* id 101, degC */
  sVarF32 BoilerOut_C;     /* id 102, degC */
  sVarF32 TankInHot_C;     /* id 103, degC */
  sVarF32 TankOutHot_C;    /* id 104, degC */
  sVarF32 Tank1_C;         /* id 105, degC */
  sVarF32 Tank2_C;         /* id 106, degC */
  sVarF32 Tank3_C;         /* id 107, degC */
  sVarF32 Tank4_C;         /* id 108, degC */
  sVarF32 Tank5_C;         /* id 109, degC */
  sVarF32 Tank6_C;         /* id 110, degC */
  sVarF32 WallIn_C;        /* id 111, degC */
  sVarF32 WallOut_C;       /* id 112, degC */
  sVarF32 BoilerExhaust_C; /* id 113, degC */
  sVarF32 RadHot_C;        /* id 114, degC */
  sVarF32 RadCold_C;       /* id 115, degC */
  sVarF32 TankInCold_C;    /* id 116, degC */
  sVarF32 TankOutCold_C;   /* id 117, degC */
  sVarF32 Board_C;         /* id 120, degC */
} sVarsTemperaturesTechm;

typedef struct
{
  sVarF32 Board_C;      /* id 121, degC */
  sVarF32 Downstairs_C; /* id 124, degC */
} sVarsTemperaturesIoboardD;

typedef struct
{
  sVarF32 Board_C; /* id 122, degC */
} sVarsTemperaturesIoboardU;

typedef struct
{
  sVarF32 Board_C;   /* id 123, degC */
  sVarF32 Office_C;  /* id 125, degC */
  sVarF32 Kidroom_C; /* id 126, degC */
  sVarF32 Outside_C; /* id 127, degC */
} sVarsTemperaturesElecon;

typedef struct
{
  sVarF32 Workshop_C;     /* id 144, degC */
  sVarF32 PackWorkshop_C; /* id 145, degC */
  sVarF32 Board_C;        /* id 146, degC */
  sVarF32 BikeRoom_C;     /* id 147, degC */
} sVarsTemperaturesEleconD;

typedef struct
{
  sVarsTemperaturesTechm    Techm;     /* TECHM, validity per field */
  sVarsTemperaturesIoboardD IoboardD;  /* IOBOARD_D, validity per field */
  sVarsTemperaturesIoboardU IoboardU;  /* IOBOARD_U, validity per field */
  sVarsTemperaturesElecon   Elecon;    /* ELECON, validity per field */
  sVarsTemperaturesEleconD  EleconD;   /* ELECON_D, validity per field */
} sVarsTemperatures;

typedef struct
{
  sVarF32 WindBurst_mps; /* id 161, m/s */
  sVarF32 WindAvg_mps;   /* id 162, m/s */
  sVarF32 WindPower_W;   /* id 163, W */
  sVarF32 WindEnergy_Wh; /* id 164, Wh */
} sVarsMeteo;

typedef struct
{
  sVarU32 LoftWest_pct;     /* id 400, % */
  sVarU32 LoftEast1_pct;    /* id 401, % */
  sVarU32 LoftEast2_pct;    /* id 402, % */
  sVarU32 BedroomSouth_pct; /* id 403, % */
  sVarU32 BedroomNorth_pct; /* id 404, % */
  sVarU32 Bathroom_pct;     /* id 405, % */
  sVarU32 StairSteps_pct;   /* id 406, % */
  sVarU32 Stairs_pct;       /* id 407, % */
  sVarU32 Hallway_pct;      /* id 408, % */
  sVarU32 LivingRoom_pct;   /* id 409, % */
  sVarU32 DiningRoom_pct;   /* id 410, % */
  sVarU32 Kitchen_pct;      /* id 411, % */
  sVarI32 ScreenEast;       /* id 412, enum eBlindCmd */
  sVarI32 ScreenWest;       /* id 413, enum eBlindCmd */
} sVarsStatesIoboardU;

typedef struct
{
  sVarU32 Washbasin_pct;      /* id 420, % */
  sVarU32 KitchenCounter_pct; /* id 421, % */
  sVarI32 SewagePump;         /* id 422, bool */
} sVarsStatesIoboardD;

typedef struct
{
  sVarI32 AcAxpert;          /* id 490, enum eInverterState */
  sVarI32 AcVictron;         /* id 491, enum eInverterState */
  sVarI32 AkupackHeater;     /* id 492, bool */
  sVarF32 AkupackSetDay_C;   /* id 493, degC */
  sVarF32 AkupackSetNight_C; /* id 494, degC */
  sVarI32 EvseRelay;         /* id 495, bool */
  sVarI32 HeatingRelay;      /* id 496, bool */
  sVarU32 ExteriorLight_pct; /* id 497, % */
  sVarU32 Kolarna_pct;       /* id 498, % */
  sVarU32 Sklep_pct;         /* id 499, % */
} sVarsStatesEleconD;

typedef struct
{
  sVarI32 Ac300W;            /* id 480, enum eInverterState */
  sVarI32 Ac3kW;             /* id 481, enum eInverterState */
  sVarI32 Ac5kW;             /* id 482, enum eInverterState */
  sVarU32 ExteriorLight_pct; /* id 483, % */
} sVarsStatesElecon;

typedef struct
{
  sVarI32 RemoteMode;      /* id 510, enum eRecuRemoteMode */
  sVarI32 Limit;           /* id 511, enum eRecuLimit */
  sVarU32 Co2Setpoint_ppm; /* id 512, ppm */
  sVarF32 AntiDryRh_pct;   /* id 513, % */
} sVarsStatesRecu;

typedef struct
{
  sVarF32 UpReqTempDay_C;     /* id 440, degC */
  sVarF32 UpReqTempNight_C;   /* id 441, degC */
  sVarF32 DownReqTempDay_C;   /* id 442, degC */
  sVarF32 DownReqTempNight_C; /* id 443, degC */
  sVarF32 ExhaustSetpoint_C;  /* id 444, degC */
  sVarF32 UpReqTemp_C;        /* id 445, degC */
  sVarF32 DownReqTemp_C;      /* id 446, degC */
  sVarI32 RadOn;              /* id 447, bool */
  sVarI32 WallOn;             /* id 448, bool */
  sVarI32 PumpBoiler;         /* id 449, bool */
  sVarI32 PumpWall;           /* id 450, bool */
  sVarI32 PumpRad;            /* id 451, bool */
  sVarU32 ElHeaterCoils;      /* id 452 */
  sVarI32 BoilerState;        /* id 453, enum eBoilerStatus */
  sVarF32 ValveWall_pct;      /* id 460, % */
  sVarF32 ValveRad_pct;       /* id 461, % */
  sVarF32 AirValveSignal_pct; /* id 462, % */
  sVarF32 ReqWaterRad_C;      /* id 463, degC */
  sVarF32 ReqWaterWall_C;     /* id 464, degC */
} sVarsStatesTechm;

typedef struct
{
  sVarsStatesIoboardU IoboardU;  /* IOBOARD_U */
  sVarsStatesIoboardD IoboardD;  /* IOBOARD_D */
  sVarsStatesEleconD  EleconD;   /* ELECON_D */
  sVarsStatesElecon   Elecon;    /* ELECON */
  sVarsStatesRecu     Recu;      /* RECU */
  sVarsStatesTechm    Techm;     /* TECHM */
} sVarsStates;

typedef struct
{
  sVarsSystem       System;             /* ELECON */
  sVarsGlobalEnergy GlobalEnergy;       /* ELECON */
  sVarsPack         PackWorkshop251Ah;  /* ELECON_D */
  sVarsPack         PackHome220Ah;      /* ELECON */
  sVarsPack         PackHome80Ah;       /* ELECON */
  sVarsMppt         Mppt;               /* ELECON */
  sVarsShunt        ShuntWorkshop;      /* ELECON_D */
  sVarsShunt        ShuntHome;          /* ELECON */
  sVarsLocalEnergy  HomeEnergy;         /* ELECON */
  sVarsLocalEnergy  WorkshopEnergy;     /* ELECON_D */
  sVarsAxpert       Axpert;             /* ELECON_D */
  sVarsMeters       Meters;             /* producer per subgroup */
  sVarsElHeater     ElHeater;           /* TECHM */
  sVarsTank         Tank;               /* TECHM */
  sVarsBoiler       Boiler;             /* TECHM */
  sVarsWater        Water;              /* TECHM */
  sVarsRecu         Recu;               /* RECU */
  sVarsTemperatures Temperatures;       /* producer per subgroup */
  sVarsMeteo        Meteo;              /* IOBOARD_U */
  sVarsStates       States;             /* producer per subgroup */
} sVars;

extern sVars Vars2;
extern const sVarMeta VarMeta[];        /* VARS_COUNT entries, sorted by id */
extern const uint16_t VarIdToIndex[];   /* VAR_MAX_ID + 1 entries, VAR_IDX_UNKNOWN = unknown id */

#endif /* UHA_VARS_GEN_H_ */
