# UHA_COMMON — Shared Library

## What This Is
Shared C library linked into every ASW project via STM32CubeIDE linked resources (`PARENT-1-PROJECT_LOC/UHA_COMMON/`). **Edit files here directly** — the `Common/` folder inside each ASW project is a virtual folder pointing here.

---

## Module Reference

### VARS — Global Variable Bus
**Files:** `VARS.h` / `VARS.c`

Central registry of all inter-node data. ~400 integer IDs, `int16_t` values.

```c
typedef struct { uint8_t valid; uint8_t local; int16_t value; } sVar;

VAR_Init()
VAR_GetVariable(varId, *invalid)
VAR_SetVariable(varId, value, valid)
VAR_GetVariablePointer(varId, *invalid)
```

**ID ranges:**
| Range | Subsystem |
|-------|-----------|
| 1 | Network status |
| 8–18 | Solar/PV aggregates |
| 10–18 | Battery aggregates (SOC, voltage, current, power) |
| 20–24 | BMS1 summary (workshop pack, published by ELECON_D) |
| 30–34 | BMS2 summary (house pack 1, published by ELECON) |
| 35–39 | BMS3 summary (house pack 2, published by ELECON) |
| 40–51 | MPPT controller, shunts |
| 60–93 | AC circuit consumption/power (AC300, AC3KW, fridge, kitchen, etc.) |
| 80–99 | Electric heater, heating energy |
| 100–152 | All temperature sensors (boiler, tanks, rooms, RECU, board temps, outdoor) |
| 161–169 | Meteo (wind speed, gust, power, energy, rainfall) |
| 180–211 | BMS1 cells: voltages (MV, 16 cells) + temps (C, 16 cells) |
| 220–251 | BMS2 cells: voltages + temps |
| 260–295 | Axpert inverter data |
| 300–317 | Storage 1 (STRG1): SOC, power, local PV/loads |
| 320–337 | Storage 2 (STRG2): SOC, power, local PV/loads |
| 340–344 | LVDC (low-voltage DC) loss/efficiency |
| 350–381 | BMS3 cells: voltages (MV, 16 cells) + temps (C, 16 cells) |

**NUM_OF_VARIABLES = 400**

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

High-level bridge between MCAN and application. Handles variable streaming, button events, gestures, RTC sync, system info.

```c
COM_Init(nodeId)
COM_Update_10ms()
COM_SendVariable(varId)
COM_AddStreamedVariable(varId, period_ms)   // up to 100 streams
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
| 0x110 | Variable value (read/write) |
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

Pulse-counting AC energy metering. Maps GPIO pins to VARS IDs.

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
