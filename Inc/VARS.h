/*
 * VARS.h
 *
 *  Created on: Jul 13, 2021
 *      Author: Martin
 */

#ifndef INC_VARS_H_
#define INC_VARS_H_

#include "main.h"


#define  VAR_NETWORK_STATUS  1

#define  VAR_BAT_SOC  10
#define  VAR_BAT_VOLTAGE_V10  11
#define  VAR_LOAD_A10  12
#define  VAR_CHARGING_A10  13
#define  VAR_BAT_CURRENT_A10  14

#define  VAR_BMS1_SOC  20
#define  VAR_BMS1_CURRENT_A10  21
#define  VAR_BMS1_VOLTAGE_V10  22
#define  VAR_BMS1_ENERGY_STORED_WH  23
#define  VAR_BMS1_TODAY_ENERGY_WH  24

#define  VAR_BMS2_SOC  30
#define  VAR_BMS2_CURRENT_A10  31
#define  VAR_BMS2_VOLTAGE_V10  32
#define  VAR_BMS2_ENERGY_STORED_WH  33
#define  VAR_BMS2_TODAY_ENERGY_WH  34

#define  VAR_MPPT_BAT_CURRENT_A10  40
#define  VAR_MPPT_BAT_VOLTAGE_V100  41
#define  VAR_MPPT_YIELD_TODAY_10WH  42
#define  VAR_MPPT_MAX_TODAY_W  43
#define  VAR_MPPT_SOLAR_POWER_W  44
#define  VAR_MPPT_SOLAR_VOLTAGE_V100  45
#define  VAR_MPPT_SOLAR_CURRENT_A10  46
#define  VAR_MPPT_SOLAR_MAX_VOLTAGE_V100  47

#define  VAR_EL_HEATER_STATUS  80
#define  VAR_EL_HEATER_POWER  81
#define  VAR_EL_HEATER_CURRENT  82
#define  VAR_EL_HEATER_CONS  83
#define  VAR_FLOW_COLD  90
#define  VAR_FLOW_HOT  91
#define  VAR_CONS_COLD  92
#define  VAR_CONS_HOT  93
#define  VAR_TEMP_BOILER  100
#define  VAR_TEMP_BOILER_IN  101
#define  VAR_TEMP_BOILER_OUT  102
#define  VAR_TEMP_TANK_IN_H  103
#define  VAR_TEMP_TANK_OUT_H  104
#define  VAR_TEMP_TANK_1  105
#define  VAR_TEMP_TANK_2  106
#define  VAR_TEMP_TANK_3  107
#define  VAR_TEMP_TANK_4  108
#define  VAR_TEMP_TANK_5  109
#define  VAR_TEMP_TANK_6  110
#define  VAR_TEMP_WALL_IN  111
#define  VAR_TEMP_WALL_OUT  112
#define  VAR_TEMP_BOILER_EXHAUST  113
#define  VAR_TEMP_RAD_H  114
#define  VAR_TEMP_RAD_C  115
#define  VAR_TEMP_TANK_IN_C  116
#define  VAR_TEMP_TANK_OUT_C  117
#define  VAR_TEMP_TECHM_BOARD  120
#define  VAR_TEMP_IOBOARD_D  121
#define  VAR_TEMP_IOBOARD_U  122
#define  VAR_TEMP_ELECON_BOARD  123
#define  VAR_TEMP_DOWNSTAIRS  124
#define  VAR_TEMP_OFFICE  125
#define  VAR_TEMP_KIDROOM  126
#define  VAR_TEMP_OUTSIDE  127


#define  VAR_BMS1_CELL1_MV  180
#define  VAR_BMS1_CELL2_MV  181
#define  VAR_BMS1_CELL3_MV  182
#define  VAR_BMS1_CELL4_MV  183
#define  VAR_BMS1_CELL5_MV  184
#define  VAR_BMS1_CELL6_MV  185
#define  VAR_BMS1_CELL7_MV  186
#define  VAR_BMS1_CELL8_MV  187
#define  VAR_BMS1_CELL9_MV  188
#define  VAR_BMS1_CELL10_MV  189
#define  VAR_BMS1_CELL11_MV  190
#define  VAR_BMS1_CELL12_MV  191
#define  VAR_BMS1_CELL13_MV  192
#define  VAR_BMS1_CELL14_MV  193
#define  VAR_BMS1_CELL15_MV  194
#define  VAR_BMS1_CELL16_MV  195
#define  VAR_BMS1_CELL1_C  196
#define  VAR_BMS1_CELL2_C  197
#define  VAR_BMS1_CELL3_C  198
#define  VAR_BMS1_CELL4_C  199
#define  VAR_BMS1_CELL5_C  200
#define  VAR_BMS1_CELL6_C  201
#define  VAR_BMS1_CELL7_C  202
#define  VAR_BMS1_CELL8_C  203
#define  VAR_BMS1_CELL9_C  204
#define  VAR_BMS1_CELL10_C  205
#define  VAR_BMS1_CELL11_C  206
#define  VAR_BMS1_CELL12_C  207
#define  VAR_BMS1_CELL13_C  208
#define  VAR_BMS1_CELL14_C  209
#define  VAR_BMS1_CELL15_C  210
#define  VAR_BMS1_CELL16_C  211


#define  VAR_BMS2_CELL1_MV  220
#define  VAR_BMS2_CELL2_MV  221
#define  VAR_BMS2_CELL3_MV  222
#define  VAR_BMS2_CELL4_MV  223
#define  VAR_BMS2_CELL5_MV  224
#define  VAR_BMS2_CELL6_MV  225
#define  VAR_BMS2_CELL7_MV  226
#define  VAR_BMS2_CELL8_MV  227
#define  VAR_BMS2_CELL9_MV  228
#define  VAR_BMS2_CELL10_MV  229
#define  VAR_BMS2_CELL11_MV  230
#define  VAR_BMS2_CELL12_MV  231
#define  VAR_BMS2_CELL13_MV  232
#define  VAR_BMS2_CELL14_MV  233
#define  VAR_BMS2_CELL15_MV  234
#define  VAR_BMS2_CELL16_MV  235
#define  VAR_BMS2_CELL1_C  236
#define  VAR_BMS2_CELL2_C  237
#define  VAR_BMS2_CELL3_C  238
#define  VAR_BMS2_CELL4_C  239
#define  VAR_BMS2_CELL5_C  240
#define  VAR_BMS2_CELL6_C  241
#define  VAR_BMS2_CELL7_C  242
#define  VAR_BMS2_CELL8_C  243
#define  VAR_BMS2_CELL9_C  244
#define  VAR_BMS2_CELL10_C  245
#define  VAR_BMS2_CELL11_C  246
#define  VAR_BMS2_CELL12_C  247
#define  VAR_BMS2_CELL13_C  248
#define  VAR_BMS2_CELL14_C  249
#define  VAR_BMS2_CELL15_C  250
#define  VAR_BMS2_CELL16_C  251








#define NUM_OF_VARIABLES		255


#define INVALID_FLAG				0x01

typedef struct
{
	uint8_t valid;
	uint8_t local;
	int16_t value;
} sVar;


void VAR_Init(void);

int16_t VAR_GetVariable(uint16_t varId, uint16_t* invalid);
int16_t* VAR_GetVariablePointer(uint8_t varId, uint16_t* invalid);
void VAR_SetVariable(uint8_t varId, int16_t value, uint8_t valid);

#endif /* INC_VARS_H_ */
