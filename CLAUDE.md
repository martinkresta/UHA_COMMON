# UHA_COMMON — Shared Library

## What This Is
Shared C library linked into every ASW project via STM32CubeIDE linked resources (`PARENT-1-PROJECT_LOC/UHA_COMMON/`). **Edit files here directly** — the `Common/` folder inside each ASW project is a virtual folder pointing here.

---

## Module Reference

### VARS2 — Shared Variables
**Files:** `VARS2.h` / `VARS2.c` (hand-written), `VARS2_types.h`, generated `uha_vars_gen.h/.c` from
`VarDefs/uha_vars.yaml` (`python VarDefs/tools/gen_all.py`). Spec: `VarDefs/VARS2_spec.md`.

All inter-node data lives in `Vars2`, grouped as in the YAML: `Vars2.<Group>.<Field>` or
`Vars2.<Container>.<Group>.<Field>`. Every field is `{value, valid}` (`sVarF32` / `sVarI32` / `sVarU32`),
value in physical units (`Voltage_V`, `Soc_pct`, ...). CAN frame `0x180 + node`: id, 4-byte value, valid flag.

```c
// APP.c wires the groups: produced = non-const, read = const
GEMON_Init(&Vars2.GlobalEnergy, &Vars2.HomeEnergy, &Vars2.WorkshopEnergy);

// producer
p->Soc_pct.value = soc;  p->Voltage_V.value = v;
VARS2_SetGroupValid(p, valid);            // all fields of the group / container
VarSet(&p->Current_A, current, valid);    // one field (VarSetI32 / VarSetU32)
VARS2_SetById(VID_TEMP_OUTSIDE_C, t, 1);  // by id (TEMP, ELM, SENSIRION)

// reader
if (pBat->Soc_pct.valid) { ... pBat->Soc_pct.value ... }
```

VARS2 sends every own variable by its YAML `period_ms` (no stream lists; `send: on_change` variables also on change, min 1 s apart) and clears `.valid` of received
variables after `timeout_ms`. Ids (`VID_*`) and names are fixed (wire format, UhaGatePy, PC tool).

**Id ranges** (unchanged):
| Range | Subsystem |
|-------|-----------|
| 1 | Network status |
| 8–18 | Global energy (battery, solar, load) |
| 20–24 | Workshop pack 251 Ah (ELECON_D) |
| 30–34 | House pack 220 Ah (ELECON) |
| 35–39 | House pack 80 Ah (ELECON) |
| 40–51 | MPPT controller, shunts |
| 60–79 | AC circuit consumption / power (ELM) |
| 80–99 | Electric heater, heating energy, water |
| 100–152 | Temperatures (boiler, tanks, rooms, RECU, boards, outdoor) |
| 161–169 | Meteo |
| 180–211 | Workshop pack cells: voltages + temps |
| 220–251 | House pack 220 Ah cells |
| 260–295 | Axpert inverter, workshop meters |
| 300–337 | Home / workshop energy (storage, load, PV) |
| 350–381 | House pack 80 Ah cells |
| 400–519 | States (`send: on_change`): IOBOARD_U 400–419, IOBOARD_D 420–439, TECHM 440–479, ELECON 480–489, ELECON_D 490–509, RECU 510–519 |

---

### MCAN — CAN Driver + NMT State Machine
**Files:** `MCAN.h` / `MCAN.c`

CANopen-style protocol stack. Manages RX/TX buffers, heartbeat monitoring, node tracking.

```c
MCAN_Init(can_hw, node_id)
MCAN_Update_10ms()          // heartbeat + timeout detection
MCAN_SendFrame(cobid, data, dlc)
MCAN_GetRxMessage(*msg)
MCAN_GetNodeStatus(nodeId)  // alive/dead
MCAN_Transmit()             // flush TX queue (called 5ms)
```

**NMT states:** BOOTUP → STOPPED → PREOPERATIONAL → RUN  
**Buffers:** RX0/RX1 = 80 msgs each, TX = 100 msgs  
**Node IDs:** 1=IOBOARD_D, 2=IOBOARD_U, 3=TECHM, 4=ELECON, 5=RECU, 6=EVSE, 7=ELECON_D

---

### COM — CAN Message Dispatcher
**Files:** `COM.h` / `COM.c`

High-level bridge between MCAN and application: VARS2 RX/TX (via `VARS2_Update_10ms()`), button events, gestures, RTC sync, system info.

```c
COM_Init(nodeId)
COM_Update_10ms()
COM_SendMessage(cobid, data, dlc)
COM_SendGesture(gestureId)
COM_SendACRemoteRequest(type, req, keepTime)
COM_SendRecuRemoteRequest(mode, duration)
COM_SendLogMsg(event, value)
COM_GetRxMessage(&msg)
```

**Key CAN COBIDs:**
| COBID | Message |
|-------|---------|
| 0x100 | Button state |
| 0x180 | VARS2 variable frame (`0x180 + node`), see `VarDefs/VARS2_spec.md` |
| 0x120 | Gesture command |
| 0x130 | AC remote request |
| 0x140 | RECU remote request |
| 0x510 | RTC sync |
| 0x550 / 0x560 | System info request / response |
| 0x620 | Log message |
| 0x600–0x610 | UHAMON remote monitoring |

---

### BMS — JK Smart BMS Driver
**Files:** `BMS.h` / `BMS.c`

Non-blocking UART driver for JK BMS protocol. Supports up to 2 packs simultaneously via `BMS_UartRxCallback`.

```c
BMS_Init(bms)
BMS_Update(bms)                      // call periodically
BMS_GetMaxCellVoltage(bms)
BMS_GetMinCellVoltage(bms)
BMS_IsChargingEnabled(bms)
BMS_IsDischargingEnabled(bms)
BMS_UartRxCallback(huart, len, bms1, bms2)
```

**Status flags:** ALLOW_CHARGE, ALLOW_DISCHARGE, COM_ERROR, CELL_LOW/HIGH_VOLTAGE, LOW/HIGH_TEMPERATURE, SOC_UNKNOWN

---

### ELM — AC Energy Meter
**Files:** `ELM.h` / `ELM.c`

Pulse-counting AC energy metering. Maps GPIO pins to VARS2 ids (`VID_*`).

```c
ELM_Init(numOfMeters)
ELM_AddMeter(id, gpio_pin, var_id)
ELM_Update_1s()
ELM_ExtiCallback(pin)                // call from GPIO ISR
ELM_GetConsumptionWh(id)
ELM_GetPowerW(id)
ELM_ResetConsumption(id)
```

---

### TEMP — 1-Wire Temperature Sensors
**Files:** `TEMP.h` / `TEMP.c`

Manages up to 41 DS18B20 sensors across multiple 1-Wire buses.

```c
TEMP_Init()
TEMP_AddHwBus(busId, gpio_port, gpio_pin)
TEMP_AssignSensor(sensorId, varId, busId)
TEMP_Update100ms()
```

Invalidation: 5 consecutive errors → sensor marked invalid.

---

### OW — 1-Wire Bus Controller
**Files:** `OW.h` / `OW.c`

Low-level bit-bang 1-Wire protocol. Uses TIM6 for µs-precision timing. ISR-driven state machine.

```c
OW_Init()
OW_AddBus(busId, gpio_port, gpio_pin)
OW_ReadSensor(busId, *rom, *result, *valid)
OW_ConvertAll(busId)
OW_IRQHandler()
```

---

### SENSIRION — I2C Environmental Sensors
**Files:** `SENSIRION.h` / `SENSIRION.c`

Supports SHT4x (temp+RH), SCD4x (CO₂), SDP810 (differential pressure).

```c
SENS_Init()
SENS_AddSensor(sensor)               // up to 10 sensors
SENS_ReadSensor(*sensor)
SENS_Update_1s()
```

---

### METEO — Wind & Rain Sensors
**Files:** `METEO.h` / `METEO.c`

Anemometer + rain gauge via pulse interrupts. Computes 30s average, gust, and wind power.

```c
METEO_Init(windPin, rainPin)
METEO_Update_1s()
METEO_ExtiCallback(pin)
METEO_GetWindSpeedAvg() / Burst() / GetWindEnergyWh()
METEO_ResetWindEnergy() / ResetRainFall()
```

---

### PID — Generic PID Controller
**Files:** `PID.h` / `PID.c`

Float-based PID with output clamping.

```c
PID_Init(pid)
PID_Update(pid, error)   // returns float action
```

Fields: `period_s`, `pFactor/iFactor/dFactor`, `minAction/maxAction`

---

### RTC — Real-Time Clock
**Files:** `RTC.h` / `RTC.c`

```c
RTC_GetTime()            // returns sDateTime
RTC_SetUnixTime(unix)
RTC_GetUnixTime()
RTC_DataReceived(data)   // parse CAN sync packet
RTC_PrintTimeStamp(buf)
```

---

### GEST — Gesture Recognition
**Files:** `GEST.h` / `GEST.c`

Multi-button / multi-touch gesture detection. Up to 20 registered gestures.

```c
GEST_Init()
GEST_AddGesture(gesture_init)
GEST_DiInputChanged(inputId, state)  // call from DI driver
GEST_Update_10ms()
```

Gesture IDs include: AC5KW/AC3KW toggles, ALL_LIGHTS_OFF, WSCRC_EAST/WEST, STOVE controls, AXPERT toggle.

---

### UHAMON — Remote Monitoring Protocol
**Files:** `UHAMON.h` / `UHAMON.c`

Type-less memory read/write via 8-byte CAN frames. Used by PC CanMaster v2 application.

```c
UHAMON_Init()
UHAMON_ProcessInput(p_data)
UHAMON_SetSendFunction(f_send)
```

Commands: 0x31 (set addr), 0x32/33/34 (write 8/16/32-bit), 0x38 (read var), 0x39 (read param)

---

### LOG — Event Logging
**Files:** `LOG.h` / `LOG.c`

```c
LOG_Startup()
LOG_SaveToBackup(reg, value)    // persist to RTC backup registers
```

Events: eleStartup, eleError, eleLog, eleStatus

---

### UI — User Interface
**Files:** `UI.h` / `UI.c`

RGB LED + life LED + buzzer control with pattern modes.

```c
UI_Init(hw_pins)
UI_Update_10ms()
UI_LED_R/G/B/Life_SetMode(status)
UI_Buzzer_SetMode(status)
```

Modes: OFF, ON, BLINKING_SLOW, BLINKING_FAST, BLINK_ONCE, BEEP_ONCE, FLASH, COMBI_1, TOGGLE

---

### circbuf — Circular Buffer
**Files:** `circbuf.h` / `circbuf.c`

Generic, element-size-agnostic circular buffer.

```c
CB_Create(elementsize, length)  // returns CB_handle
CB_Put(cb, item)                // enqueue (overwrites if full)
CB_Get(cb, item)                // dequeue
CB_Probe(cb, item)              // peek (non-destructive)
CB_NumOfItems(cb)
```

---

### watchdog — IWDG Wrapper
**Files:** `watchdog.h` / `watchdog.c`

```c
WDG_Init(ms)       // configure timeout
WDG_Refresh()      // call before timeout (typically in 1s task)
```
