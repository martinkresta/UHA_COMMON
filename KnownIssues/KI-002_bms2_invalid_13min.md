# KI-002: BMS2 (House 220 Ah) invalid for 13 min, recovered by itself

| | |
|---|---|
| Status | open |
| Severity | medium |
| Area | ASW_ELECON (node 4), UART2 -> BMS 220 Ah (`UHA_COMMON/Src/BMS.c` via `LEMON.c`) / BMS 220 Ah itself |
| First seen | 2026-11-15 10:46 |
| Occurrences | 2026-11-15 10:46:10 - 11:04:00 (13 min fully invalid, then 4 min intermittent) |

## Symptom

In Grafana all BMS2 series (SOC, current, voltage, energy, 16 cell voltages, 16 cell temperatures) stopped at
10:46. All other ELECON data (BMS3, power, energy meters, temperatures) kept going. It came back by itself, nobody
reset ELECON or the BMS.

## Evidence

- VARS2 log DB: last valid 10-s slot 10:46:00, then every BMS2 slot NULL until 10:59:30.
- Recovery was not clean. Single valid 10-s slots appeared at 10:59:40, 11:00:30, 11:02:10, 11:02:50, 11:03:00,
  11:03:30 and 11:03:40. The data has been continuously valid since 11:04:00.
- The tunnel packets for BMS2 **kept arriving** at the normal rate (age 0-3 s in `uha2.json`), with flag
  VALID = 0. The values were frozen at the last good frame (e.g. `BMS2_CURRENT_A` = 40.25 A constant) while BMS3 and
  `BAT_POWER_W` kept changing.
- `log.txt`: no STARTUP of ELECON (last one 14.11. 15:49), so it was not a reset. Gateway journal: no errors, only
  `VARS2 stream: 213 of 258 fresh; invalid 41 (BMS2_SOC, ...)` at 10:52 and 11:02.

## Analysis

ELECON marks the whole `PackHome220Ah` group invalid when `BMS_Update()` has run `BMS_TIMEOUT_S` (5) times without
a valid frame (`BMS.c`: length 58/59 B and checksum OK). The last values are kept and sent with VALID = 0, which is
what we saw.

So from 10:46 to 10:59 ELECON received **no valid frame** on UART2. In the next 4 min it got only occasional valid
frames. The gateway and the tunnel are not involved.

Possible causes, not yet told apart:
1. The BMS 220 Ah stopped sending or sent damaged frames (BMS FW glitch, sleep, its own comm board).
2. Line or cable / level converter problem on UART2 (intermittent at the end fits this or 1).
3. ELECON UART2 receive stuck. `BMS_Update()` re-arms `HAL_UARTEx_ReceiveToIdle_DMA` each call but ignores the
   return value and the error (`UartError` is read and dropped). Recovery without a reset makes this less likely,
   but a UART error state (overrun / framing / noise) that clears later is possible. Invalid checksums and wrong
   lengths are silently ignored (`// TBD, report invalid checksum`), so there is no counter to look at.

## How to detect it next time

- Grafana: gap in the BMS2 lines.
- Journal: `VARS2 stream: ... invalid N (BMS2_...)`.
- DB query in `README.md` (NULL slots of BMS2).

## Workaround

None needed, it recovered. If it does not, check the BMS (display / app), then reset ELECON (NMT `81 04`).

## Proposed fix / next steps

- ELECON FW: count per BMS and publish (VARS2 diagnostic vars or the SysInfo / log message):
  - frames OK
  - checksum errors
  - wrong length
  - UART errors (`HAL_UART_ErrorCallback`, ORE/FE/NE)
  - re-arm failures (`HAL_UARTEx_ReceiveToIdle_DMA` != HAL_OK)

  Next time it will show whether bytes still arrive (BMS side) or not (line / UART).
- Send a CMD_LOG_MSG event when a BMS goes invalid / valid again, so it appears in `log.txt` with the exact time.
- If it repeats: look at the BMS 220 Ah side (its log / app) at the same time.
- Related: [KI-003](KI-003_bms3_periodic_invalid.md) (the other BMS on the same ELECON),
  [KI-004](KI-004_elecon_bms_active_never_cleared.md) (what ELECON does with the stale data meanwhile).
