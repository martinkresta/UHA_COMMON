/*
 * CMD.c - command channel, node side (UHA_COMMON/VarDefs/CMD_spec.md section 4)
 *
 *  Called from COM_GetRxMessage() for CAN frames 0x200 + own node, and on ELECON from SCOM.c for RPi
 *  commands targeted at ELECON itself. Dispatches to the node's APP_OnCommand().
 */

#include "CMD.h"
#include "COM.h"      /* COM_GetNodeId() */
#include <string.h>


static const sCmdMeta* FindCmd(uint16_t id)
{
	uint16_t i;
	for (i = 0; i < CMDS_COUNT; i++)
	{
		if (CmdMeta[i].Id == id)
		{
			return &CmdMeta[i];
		}
	}
	return 0;
}


uint8_t CMD_ProcessFrame(const uint8_t* data)
{
	const sCmdMeta* m;
	uint16_t id;
	uint32_t raw;
	float value;

	id = (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
	m = FindCmd(id);
	if (m == 0 || m->Target != COM_GetNodeId())
	{
		return 0;   /* unknown id, or a command for another node */
	}

	raw = (uint32_t)data[2] | ((uint32_t)data[3] << 8) | ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 24);
	switch (m->Type)
	{
		case VT_FLOAT:
			memcpy(&value, &raw, sizeof(value));
			if (!(value == value) || value > 3.4e38f || value < -3.4e38f)
			{
				return 0;   /* NaN / inf: not a value */
			}
			break;
		case VT_UINT32:
			value = (float)raw;
			break;
		default:   /* VT_INT32, VT_BOOL, VT_ENUM */
			value = (float)(int32_t)raw;
			break;
	}

	APP_OnCommand((eCmdId)id, value);
	return 1;
}


float CMD_Clamp(float value, float min, float max)
{
	if (!(value >= min))   /* also NaN */
	{
		return min;
	}
	if (value > max)
	{
		return max;
	}
	return value;
}


/* default for nodes without commands; a node's APP.c overrides it */
__attribute__((weak)) void APP_OnCommand(eCmdId id, float value)
{
	(void)id;
	(void)value;
}
