# KI-001: ELECON / ELECON_D stop transmitting on CAN after days

| | |
|---|---|
| Status | open (root cause unknown) |
| Severity | high |
| Area | ASW_ELECON (node 4), ASW_ELECON_D (node 7): CAN TX (`MCAN_Transmit`, TX ring) |
| First seen | 2026-11-12 ~10:00 |
| Occurrences | 2026-11-12 ~10:00 - 16:38 (6.5 h, until NMT reset) |

## Symptom

After ~4 days of uptime ELECON and ELECON_D stopped **transmitting** on CAN. They still received frames and
accepted commands. TECHM showed `EL_HEATER_STATUS = 8` (EleconComError). ELECON_D's last VARS2 packet was 10:00:56.

## Evidence / why it was hard to see

ELECON kept re-sending its cached copy of ELECON_D's old values over serial with valid = 1. As a result the frozen
ELECON_D data (BMS1, workshop temperatures) looked fresh ("age 0 s") in `uha.json` for 6.5 h.

Only reliable signs:
- all variables of a producer constant for minutes;
- `Nodes.<NODE>.AgeS` growing in `uha_status.json`;
- `EL_HEATER_STATUS = 8`.

## Analysis

Not found. Suspicion (unverified): a TX queue / CAN TX stall in the FW after the dual old + VARS2 transmission
doubled the frame rate. `MCAN_Transmit` sends one frame per call; the TX ring holds 100 frames. A bus-off or
error-passive state that is never recovered is also possible.

## How to detect it next time

- UhaGatePy frozen-node watch: journal `node X FROZEN`, `uha_status.json` `Nodes.X.Frozen`, `WS/uha-status.html`
  (deployed 2026-11-12). It catches nodes whose values stop moving. It does not catch ELECON's own TX failure
  if ELECON keeps feeding the serial link.
- SysInfo uptime cannot be read from a node that does not transmit.

## Workaround

NMT reset over CAN: COB-ID `0x000`, DLC 2, data `81 <node id>` (`81 04` ELECON, `81 07` ELECON_D), or
`UhaOtaCli reset --node N`. The UhaMaster GUI reset button is disabled for offline nodes
(`NetStateControl.xaml:85`, `IsEnabled="{Binding IsOnline}"`), which is not yet fixed.

## Proposed fix / next steps

- In FW: monitor the CAN error state (TEC/REC, bus-off, error passive) and TX ring fill level. Recover
  automatically (re-init FDCAN / bxCAN on bus-off or on a TX ring stuck full for > N s). Log the event.
- Publish the counters (TX ring high-water mark, TX drops, bus-off count) as diagnostic variables.
- Enable the UhaMaster reset button for offline nodes.
