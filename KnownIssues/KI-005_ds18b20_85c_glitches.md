# KI-005: DS18B20 sensors read 85.0 °C (power-on value) for single readouts

| | |
|---|---|
| Status | open — filter + diagnostics in ELECON v002.02 and IOBOARD_D v002.02 (flashed 2026-11-17), cause not known yet |
| Severity | medium |
| Area | FW UHA_COMMON `TEMP.c` / `OW.c`; seen on ELECON and IOBOARD_D |
| First seen | 2026-11-15 08:00 (first day of the DB log; probably older) |
| Occurrences | ELECON: ~1–7 per hour from 15.11 08:00 until 16.11 07:53; after the restart at 16.11 07:55 none for 6.6 h, then 1–4 per hour (rising) until the flash on 17.11. IOBOARD_D: 13 (board) + 8 (downstairs) in 26 h |

## Symptom
Grafana / `raw` table: short upward spikes of all temperatures of one node at the same time (ELECON: board, office,
kidroom, outside +3…+4 °C in the 60 s average, sometimes 2–4× that). Back to normal in the next minute.

## Evidence
- Back-calculated from the 60 s averages (20 samples of the 3 s period per minute): **every bad sample is exactly
  85.0 °C**, for every sensor and every spike; larger spikes are 2–4 bad samples in one minute.
- `OW.c` checks the CRC: the 85.0 °C readouts have a **valid CRC** — real scratchpads, not transfer errors.
- ELECON: usually all 4 sensors at once (onboard + 3 cables), sometimes only 3. IOBOARD_D: the **onboard** sensor
  too, together with the 1 m cable sensor in 6 cases, alone in the others.
- TECHM (18 sensors, ~10 m star), ELECON_D, IOBOARD_U: no spikes at all.
- No correlation with inverter relay switching, load, battery current or PV jumps (all at or below chance level).
- ELECON: ~4 per hour all night, none after the restart at 07:55 (flash of v002.01, sensors not power-cycled).

## Analysis
- 85.0 °C (scratchpad 0x0550) is the DS18B20 power-on value: the sensor has not finished a conversion since its last
  reset. A Convert command that was only missed gives the previous value, not 85.
- Sensors are 3-wire, 3V3 from the board (Martin). Parasite power is ruled out; a supply dip would hit the onboard
  and the cable sensor of IOBOARD_D together, which is not always the case.
- `TEMP.c` accepted −30…125 °C with a valid CRC, so 85.0 passed. Its 5-error invalidation only counts failed reads.
- Ruled out in the code: TIM6 bit timing is correct on all nodes (10 MHz), the OW interrupt has priority 0 (on ELECON
  shared only with SysTick), interrupt-disabled sections are a few µs, the read comes 2.1 s after Convert (needs 0.75 s).
- Open: why the sensors (or some of them) lose their conversion; why a restart (no sensor power cycle) stopped it on
  ELECON — this points to the MCU side and something that builds up with uptime.

## How to detect it next time
ELECON v002.02 logs every ignored readout (max one pair per minute) as node log messages, journal
`node event: ELECON LOG ...` and `/home/pi/Web/log.txt`:
- `LOG 33 <sensorId> <count lo> <count hi> <uptime min lo> <uptime min hi>` (0x21: total count of ignored 85.0 readouts)
- `LOG 34 <sensorId> <TH> <config> <byte6> <byte7>` (0x22: scratchpad bytes; a power-on scratchpad is TH 75 (0x4B),
  config 127 (0x7F), 12, 16)

## Workaround
`TEMP.c` (UHA_COMMON, from ELECON v002.02; the other nodes get it with their next build): a readout of exactly
0x0550 is ignored (the last value stays) unless the last valid value is within 80.0…90.0 °C or 85.0 came 3 times in a
row (a real, steady 85 °C, e.g. TECHM tank).

## Proposed fix / next steps
1. Watch the log messages for days: does it come back with uptime? Which sensors, which scratchpad bytes?
2. If it builds up with uptime: look for a state in OW / TEMP / interrupts that changes over time.
3. Roll the filter out to IOBOARD_D (and the rest) with the next builds.
