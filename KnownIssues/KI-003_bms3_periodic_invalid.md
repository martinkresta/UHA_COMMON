# KI-003: BMS3 (House 80 Ah) invalid for ~40 s about every 54 min

| | |
|---|---|
| Status | open |
| Severity | low |
| Area | ASW_ELECON (node 4), UART3 -> BMS 80 Ah (`UHA_COMMON/Src/BMS.c` via `LEMON.c`) / BMS 80 Ah itself |
| First seen | 2026-11-15 07:53 (first day of the VARS2 log DB; may be older) |
| Occurrences | 2026-11-15 07:53:20-07:54:00, 08:47:10-08:47:50, 09:41:10-09:42:00, 10:35:20-10:35:50 |

## Symptom

Short gaps (40-50 s) in all BMS3 series, with a very regular period of about 54 min.

## Evidence

VARS2 log DB, NULL slots of the 17 BMS3 variables logged every 10 s (the 60-s ones mostly still have a valid sample
in their minute):

| from | to | variables | NULL rows |
|------|----|-----------|-----------|
| 07:53:20 | 07:54:00 | 17 | 85 |
| 08:47:10 | 08:47:50 | 17 | 77 |
| 09:41:10 | 09:42:00 | 17 | 101 |
| 10:35:20 | 10:35:50 | 17 | 68 |

Interval 53 min 50 s - 54 min. BMS1 (ELECON_D) shows no such gaps; BMS2 (same ELECON, UART2) neither, apart from
[KI-002](KI-002_bms2_invalid_13min.md).

## Analysis

Same mechanism as KI-002: no valid frame for more than `BMS_TIMEOUT_S` (5) updates -> group invalid, last values
sent with VALID = 0. The strict period points to something periodic in the BMS 80 Ah (e.g. an internal task,
balancing / calibration cycle, its own comm restart) or in ELECON (a long blocking task every ~54 min starving the
UART / scheduler - but then BMS2 would probably drop too).

## How to detect it next time

DB query in `README.md` with `BMS3%`; Grafana gaps at the times above + n * 54 min.

## Workaround

None needed (short gaps).

## Proposed fix / next steps

- Check the period over more days (does it drift, is it tied to BMS uptime, does it follow an ELECON reset?).
- The frame / error counters proposed in KI-002 would show whether frames stop or arrive damaged.
- If the BMS just pauses: consider a longer `BMS_TIMEOUT_S` for display purposes only (not for decisions).
