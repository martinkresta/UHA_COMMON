# UHA Known Issues

Problems seen on the running UHA system (all nodes, the CAN bus, the Raspberry Pi gateway, the dashboards) that are
**not solved yet**. One file per issue, so an issue can be found, linked and closed on its own.

Lives in `UHA_COMMON` because it is the one repo shared by all nodes. Issues that are purely in the Pi gateway
(`UHARPI` repo) go here too, so there is a single list.

## Index

| ID | Title | Area | Severity | Status | First seen |
|----|-------|------|----------|--------|------------|
| [KI-001](KI-001_elecon_can_tx_stall.md) | ELECON / ELECON_D stop transmitting on CAN after days | FW ELECON, ELECON_D (CAN TX) | high | open | 2026-11-12 |
| [KI-002](KI-002_bms2_invalid_13min.md) | BMS2 (House 220 Ah) invalid for 13 min, recovered by itself | FW ELECON UART2 / BMS 220 Ah | medium | open | 2026-11-15 |
| [KI-003](KI-003_bms3_periodic_invalid.md) | BMS3 (House 80 Ah) invalid for ~40 s about every 54 min | FW ELECON UART3 / BMS 80 Ah | low | open | 2026-11-15 |
| [KI-004](KI-004_elecon_bms_active_never_cleared.md) | ELECON `BMS.c`: `Active` never cleared on timeout, LEMON uses stale BMS data | FW UHA_COMMON `BMS.c` / ELECON LEMON | medium | open | 2026-11-15 |

Severity: **high** = data or control lost until someone acts; **medium** = wrong/missing data or a decision based
on stale data, recovers; **low** = cosmetic or short gaps.

## How to add an issue

Copy the template below to `KI-NNN_short_name.md` (next free number), add a row to the index. When fixed: set
Status to `fixed in <node> <version> (<date>)`, keep the file (history), move nothing.

```markdown
# KI-NNN: <title>

| | |
|---|---|
| Status | open |
| Severity | high / medium / low |
| Area | <node / module / file> |
| First seen | YYYY-MM-DD HH:MM |
| Occurrences | <date time - date time (duration)>, ... |

## Symptom
What was seen, where (Grafana, uha2.json, journal, log.txt), exact times.

## Evidence
Queries / log lines / values that show it.

## Analysis
What the code does, what is known, what is ruled out.

## Suspected cause
## How to detect it next time
## Workaround
## Proposed fix / next steps
```

## Useful tools for evidence

- VARS2 log DB on the Pi (since 2026-11-15): `/var/lib/uhalog/uha_log.db`, table `raw` (var_id, ts, value; NULL =
  no valid value in that slot), names in `vars`. Example - when was BMS2 invalid:
  ```bash
  ssh uharpi 'sudo sqlite3 -readonly /var/lib/uhalog/uha_log.db "SELECT datetime(r.ts,\"unixepoch\",\"localtime\"),
    SUM(r.value IS NULL), COUNT(*) FROM raw r JOIN vars v ON v.id=r.var_id
    WHERE v.name LIKE \"BMS2%\" AND v.log_s=10 GROUP BY r.ts HAVING SUM(r.value IS NULL)>0;"'
  ```
- Gateway journal: `journalctl -u uhagatepy` (`VARS2 stream:` every 10 min, `node X FROZEN`).
- Live values with valid flag and age: `/dev/shm/uha2.json`, node state: `/dev/shm/uha_status.json`.
- Node events (STARTUP etc.): `/home/pi/Web/log.txt`.
- Grafana: http://192.168.1.10/grafana/ (gaps in the lines = invalid data).
