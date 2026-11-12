# CAN TX stall of ELECON (node 4) and ELECON_D (node 7), 2026-11-12

Status: analysis from code + implementation on branch `feature/can_tx_supervision` (UHA_COMMON, ASW_ELECON,
ASW_ELECON_D, ASW_ELECON_TEST). Not merged. Built as ELECON v001.14 and ELECON_D v001.19 for test flashing.

## 0. Revision 2 (after review): bus-off is not proven, software stall is equally likely

Martin's objection: the nodes still received frames, and RM0394 says a bus-off bxCAN cannot receive. So a lasting
bus-off is doubtful, and ABOM alone is not a fix to rely on. What still holds is that the TX path stopped while the 5 ms
task kept running (sec. 3). But a corrupted `mMcan.TxBuf`, `mMcan.CanHw` or `hcan1.State` also stops TX silently
with RX alive: `CB_Put/CB_Probe(NULL)` do nothing, and a wrong HAL state makes `AddTxMessage` fail forever. Both
nodes got new shared code (VARS2 steps 2 and 3) on 08.11 and went quiet about 4 days later. Changes in revision 2:

- **Dead-TX rule:** no TXOK for `MCAN_TX_DEAD_MS` (12 s) is a stall even if nothing looks pending (the heartbeat
  guarantees one frame per 5 s). The 3 s pending rule stays.
- **Self-reset on by default** (`MCAN_TX_STALL_RESET_AFTER = 5`): about 18 s (pending) or 27 s (dead) without any
  transmitted frame, then `NVIC_SystemReset()`. STARTUP byte 3 = 30. This is the only cure for a state that the
  peripheral recovery cannot repair.
- **Software snapshot at the stall** (report `0x03`): HAL state, TX ring level, TME bits, TxBuf NULL, ring not empty,
  mailbox-busy count, reason. A HAL state outside READY/LISTENING is put back to LISTENING (counted).
- **Pointer guard:** `mMcan.TxBuf/Rx0Buf/Rx1Buf/CanHw` are compared every 10 ms with a shadow copy (value + inverted
  value). A difference is restored and reported (`0x04` with the corrupted value, which hints at the writer).
- **TEC/REC monitor** (report `0x02`): immediate report on a new error-warning / error-passive / bus-off (from ESR,
  or from the HAL `ErrorCode` bits that the RX interrupt sets, which also catches a short bus-off between two 10 ms
  samples), at most one per 10 s. Also a summary per 60 s window if TEC or REC peaked at >= 16 or a flag was seen.
  It reports the error types too (stuff, form, ACK, bit, CRC). TEC rising without REC points at the node's own TX
  side; both rising point at the bus.

All reports are `0x620` eleError frames: `[node, 1, code, 5 bytes]`. Codes and byte layout are in `MCAN.h`
(`MCAN_ERR_*`). ELECON sends them over SCOM, the others over CAN.

Memory layout check (deployed .map files): `mMcan` sits after GEST data in ELECON_D (0x200008b4) and after ELM
data in ELECON (0x20000700), followed by `mNodes` in both. The VARS2 data is not adjacent in either node, so a single
shared linear overrun into `mMcan` is unlikely. The TX ring itself is on the heap (`CB_Create`, malloc), between the
other early allocations. The pointer guard and the snapshot will tell next time.

## 1. Short answer (revision 1)

- **Most likely cause:** both nodes went **bus-off** and stayed there. ELECON, ELECON_D and ELECON_TEST are the only
  projects with `hcan1.Init.AutoBusOff = DISABLE` (ABOM = 0). All other nodes have `ENABLE`. No code anywhere runs
  the software bus-off recovery that the reference manual requires when ABOM = 0. So the first bus-off was permanent
  until the NMT reset.
- **Why these two nodes and nobody else:** they share a configuration (the same CubeMX CAN settings in the ELECON
  family `.ioc`), not a code path. A bus disturbance raises the TEC of every node that is transmitting at that
  moment. ELECON and ELECON_D are the two heaviest talkers (about 100 and 60 frames/s since VARS2 step 1), so they
  collect TEC fastest. Any other node that also reached bus-off recovered by itself after 128 × 11 recessive bits
  (5.6 ms at 250 kbit/s), and nobody would notice.
- **One open contradiction:** RM0394 says that a bxCAN in bus-off "is no longer able to transmit and receive
  messages". Yet both nodes still received frames, including the NMT reset. Either that sentence does not hold for
  reception in practice, or the nodes were in another state where the mailboxes stay pending. Section 4 shows that
  the code leaves no other software explanation. A 10-minute desk test (section 6, T0) settles this.
- **Fix (branch):** ABOM forced on in `MCAN_Init()` and in the three `.ioc`/`main.c` files. Also a TX liveness
  supervision: if frames are pending and none has completed for 3 s, the node recovers the CAN peripheral, counts
  it and logs a `0x620` ERROR event. A self-reset is optional and off by default. Full-ring drops are now counted,
  and cheap CAN diagnostics are available through SysInfo. The supervision works whatever the root cause is.

## 2. Facts from the code (verified)

| Item | ELECON (4) | ELECON_D (7) | ELECON_TEST | TECHM | IOBOARD_D/U | RECU | EVSE |
|---|---|---|---|---|---|---|---|
| `AutoBusOff` (ABOM) | **DISABLE** | **DISABLE** | **DISABLE** | ENABLE | ENABLE (CAN1) | ENABLE | ENABLE |
| `AutoRetransmission` | ENABLE | ENABLE | ENABLE | ENABLE | ENABLE | ENABLE | ENABLE |
| `TransmitFifoPriority` | DISABLE | DISABLE | DISABLE | DISABLE | ENABLE | ENABLE | ENABLE |
| `MCAN_Transmit()` | 5 ms task | 5 ms task | 5 ms task | 5 ms | 5 ms | 5 ms | 5 ms |
| CAN SCE IRQ in NVIC | no | no | no | no | no | no | no |
| `HAL_CAN_ErrorCallback` | not defined (HAL weak) | same | same | same | same | same | same |
| Bus-off / error recovery in SW | none | none | none | none | none | none | none |

Bit rate is 250 kbit/s (`.ioc`: `CAN1.CalculateBaudRate=250000`). IWDG: `WDG_Init(3000)`, refreshed in the 1 s task.
Deployed at the time: ELECON v001.13 (or v001.12), ELECON_D v001.18 (or v001.17), all built 2026-11-08.

## 3. TX path, step by step (identical in both nodes, all code in UHA_COMMON)

1. A producer calls `MCAN_SendFrame()` (via `COM_SendVariable` / `SendVariable` / `VARS2_SendLegacy` / heartbeat
   in `MCAN_Update_10ms`) → `Send()` → `CB_Put(TxBuf)`. The ring holds 100 frames; when it is full, `CB_Put`
   overwrites the oldest frame. Before this branch the drop was not counted or reported anywhere: `eET_TX_BUFF_WARN/FULL` exist
   in `MCAN.h` but nothing uses them. `Send()` also had no `return` statement, so the return value of
   `MCAN_SendFrame()` was undefined (nobody read it).
2. Producer rate: `COM_Update_10ms()` sends at most one streamed variable per 10 ms, so at most 2 frames / 10 ms
   since step 1. Heartbeat every 5 s (`CO_DEFAULT_HB_TO/2`), plus logs and SysInfo replies.
3. `Scheduler_Check_Flag()` (main loop) → 5 ms task → `MCAN_Transmit()`: `CB_Probe()`, then `HAL_CAN_AddTxMessage()`.
   `CB_Remove()` runs only on `HAL_OK`. One frame per call → drain capacity 200 frames/s. Measured demand: ELECON
   ~100/s, ELECON_D ~60/s.
4. `HAL_CAN_AddTxMessage()` (L4 HAL) returns an error only if (a) `hcan->State` is not READY/LISTENING, or (b) all
   three mailboxes are pending (`TME0..2 == 0`). The mailbox is then sent by the bxCAN with automatic retransmission
   (NART = 0), so the bxCAN keeps retrying a pending frame until it succeeds.
5. Nothing ever reads the TX status: no TX interrupt is enabled, and nothing reads `TSR` or `ESR`.

Consequences:
- `hcan->State` is changed only by `HAL_CAN_Init/DeInit/Start/Stop/RequestSleep/WakeUp`. Only `MCAN_Start()` calls
  one of them, once. So (a) cannot happen without memory corruption.
- The 5 ms and 10 ms tasks use the same flag mechanism in the same loop. The 10 ms task was alive (NMT reset was
  processed in `MCAN_Update_10ms()`), so `MCAN_Transmit()` was being called too.
- The races in `circbuf.c` (`CB_Put` not IRQ-protected) are self-healing: the next Put/Remove re-derives `full`, so
  they cannot fake a permanently empty ring. On the branch, the TX-ring Put now runs with IRQs masked anyway.
- **Therefore the stall was "three mailboxes pending forever" (b).** The bxCAN keeps mailboxes pending without putting
  anything on the bus in only one state: **bus-off**. In error-passive the frames are retried and appear on the
  bus. The PC adapter would ACK them, so they would succeed and show up in the PC log.

## 4. Hypotheses

| # | Hypothesis | Verdict | Reason |
|---|---|---|---|
| 1 | TX stall: `AddTxMessage` never OK again, ring fills, no recovery | **Supported**: this is what must have happened (sec. 3) | After it, the ring fills in about 1 s (ELECON) and keeps overwriting. Heartbeats go through the same ring, so they stop too. |
| 2 | Bus-off with ABOM disabled, no SW recovery | **Supported, most likely root cause** | ABOM = 0 only in the ELECON family. No INRQ cycle anywhere, so bus-off is permanent. NMT reset → `NVIC_SystemReset()` → fixed, exactly as observed. **Caveat:** RX during bus-off (see 1.); must be tested (T0). |
| 3 | TX overload since step 1 | **Ruled out as the sole cause; a contributing factor** | Demand is about 30–50% of the drain capacity, and overload only drops frames, it cannot stop TX. But twice the frames means more frames in flight during a disturbance, so the TEC grows faster. Drops were also invisible. |
| 4a | Uptime / tick dependent | **Ruled out in the TX path** | `MCAN_Transmit()` has no counters. HAL tick (uint32 ms) wraps after 49.7 d. The uint16 timeout counters in `MCAN_Update_10ms()` reset themselves. 346 040 s does not match any 2^n boundary of a counter in this path (2^28 ms = 3.1 d, 2^25 × 10 ms = 3.9 d). |
| 4b | Periodic around 10:00 | No code found | Nothing in either node is scheduled by time of day in the CAN path. An external event (welding or inverter switching in the workshop, wiring work) remains possible and fits hypothesis 2. |
| 4c | ELECON ↔ ELECON_D coupling | **Ruled out for TX** | ELECON_D only *reads* two ELECON variables (VARS2 read layer, ignores `invalid`), and ELECON reads `BMS1_*`/`AXPERT_*`. Reads never touch the TX path. |

Other cause that would end in the same state: a fault on the TX side of the node's transceiver or wiring (e.g. TXD
dominant time-out, driver disabled). The bxCAN then sees bit errors on its own frames, the TEC rises, and the node
goes bus-off. RX keeps working, because receiving needs no driver. An MCU reset would only fix this if the reset
also clears the transceiver condition.

**Side note on the 08.11. STARTUP payload "20 0 0 0 0 0":** byte 0 is `RCC->CSR >> 24`. If "20" is decimal
(0x14 = PINRSTF + SFTRSTF), it is a software reset, i.e. the bootloader after the OTA flash, which is consistent.
If it is hex (0x20 = IWDGRSTF), both nodes started from a *watchdog* reset on 08.11. That would need its own look.
Please check how the log tool prints it.

## 5. What would settle it

1. **Desk test T0** (below): does the bxCAN on the L431 still receive in bus-off with ABOM = 0? If yes, hypothesis 2
   explains every observation. If no, the nodes were in another state, and the new diagnostics will show which one.
2. PC CAN log around 12.11. 10:00:56: error frames or bus error counters of the adapter, and the exact last frame of
   ELECON_D. Did 0x707 (heartbeat) stop at the same time as 0x187? It should, because it uses the same ring.
3. ELECON's stop time: the RPi history of TECHM `EL_HEATER_STATUS` (first 8 = EleconComError), minus TECHM's
   timeout. Was it also around 10:00? If yes, one bus event hit both nodes.
4. What happened in the workshop at about 10:00: welding, inverter start/stop, Axpert remote start, wiring work?
5. Next occurrence with the new firmware: the `0x620` ERROR event (code 1) carries TEC/REC/ESR at the stall, and
   SysInfo `0x10..0x19` (section 7) can be read *after* the recovery.

## 6. Desk reproduction plan (ELECON_TEST, desk bus only; Martin flashes)

Setup: ELECON_TEST + PC CAN adapter (normal mode = ACKs) + a second node if available. Debugger attached (DBF is
cleared, so CAN keeps running while halted). Watch `hcan1.Instance->ESR`, `->TSR`, and for the new firmware also
`mDiag` (`MCAN.c`).

| Test | Firmware | Action | Expected / what it tells |
|---|---|---|---|
| T0 | current master (ABOM = 0) | Short CAN_H–CAN_L for ~0.2 s while ELECON_TEST transmits | `ESR.BOFF = 1`, TEC > 255, TSR TME0..2 = 0, PC sees nothing from the node, forever. **Then send frames + NMT reset `81 <id>` from the PC: are they received?** (breakpoint or counter in `HAL_CAN_RxFifo1MsgPendingCallback`). This is the key result. |
| T1 | current | Remove the ACK (PC adapter listen-only, no other node) | TEC climbs to 128 (error passive) and stays there; frames are retried. Restore the ACK: TX resumes by itself. Shows that "no ACK" alone does not explain the stall. |
| T2 | branch | Repeat T0 | ABOM: node back after about 6 ms. `mDiag.busOffCnt`/`tecMax` may show it (10 ms sampling). No supervision event expected. |
| T3 | branch built with `-DMCAN_FORCE_ABOM=0` | Repeat T0 | After 3 s: recovery, `0x620` frame `[node, 1, 1, 1, TEC, REC, ESR, cnt]`, TX resumes. SysInfo 0x15 shows the count. |
| T4 | branch, `-DMCAN_FORCE_ABOM=0 -DMCAN_TX_STALL_RESET_AFTER=2` | Hold the short (persistent fault) | Recovery at 3, 6, 9 s, then reset. STARTUP byte 3 = 30 (`LOG_FUNC_ID_CAN_TX_STALL`). |
| T5 | branch | Flood the bus from the PC with a higher-priority ID (e.g. 0x001, ~100% load) for 10 s | Arbitration lost, ring drops counted (SysInfo 0x13), possibly one recovery. TX resumes when the flood stops. |

## 7. The proposal (branch `feature/can_tx_supervision`)

### UHA_COMMON (affects all nodes on their next build)
- `MCAN_Init()`: sets `CAN_MCR_ABOM` (still in init mode at that point) and enables EWG/EPV/BOF notifications, so
  HAL records them in the sticky `hcan->ErrorCode`. No node enables the SCE IRQ, so this adds no interrupt load.
  Switch: `MCAN_FORCE_ABOM` (default 1).
- `MCAN_Transmit()`: before each `AddTxMessage` it reads `TSR` and counts finished mailboxes (TXOK / TERR / ALST /
  aborted), then clears RQCPx. This must happen before the mailbox is reused, because the hardware clears the flags
  on a new request. It also counts calls where no mailbox was free.
- `MCAN_Update_10ms()` → `SuperviseTx()`: tracks TEC/REC maximum and counts BOFF/EPVF edges. If frames are pending
  and no TXOK for `MCAN_TX_STALL_MS` (3000 ms; the heartbeat is every 5 s, so a healthy node always completes
  something), it runs `RecoverTx()`:
  ESR/TSR snapshot → abort mailboxes (ABRQ) → INRQ, wait INAK (bounded) → set ABOM → clear INRQ. Filters, IER, BTR
  and the HAL state stay as they are. `HAL_CAN_Stop/Start` is deliberately not used: `HAL_CAN_Start()` sets
  `HAL_CAN_STATE_ERROR` if it cannot leave init mode within 10 ms (busy or shorted bus), and every later
  `AddTxMessage` would then fail. Retries every 3 s.
- `MCAN_TxStallCallback(event, diag)`: weak in MCAN.c, implemented in `LOG.c`. It sends `0x620` eleError,
  payload `[LOG_ERR_CAN_TX_STALL=1, event, TEC, REC, ESR&0x7F, recoveries]`. On non-master nodes this is queued
  into the just-recovered ring. The master node (ELECON) sends it over SCOM, which does not depend on CAN. Before a
  self-reset it writes `LOG_FUNC_ID_CAN_TX_STALL` (30) to the RTC backup register, so the STARTUP log shows the
  reason.
- (Revision 2: the default is now 5, see section 0.) Self-reset after `MCAN_TX_STALL_RESET_AFTER` failed recoveries in a row: originally default 0 = off, because a node alone
  on the bus (no ACK) would reset itself and a reset toggles the ELECON_D relays. Suggested value if wanted: 5 (about
  18 s of silence).
- `Send()`: counts full-ring drops (`txRingDropCnt`), tracks the ring high-water mark, returns 0/1, Put under IRQ
  mask. The overwrite-oldest semantics are unchanged.
- Diagnostics: `MCAN_GetDiag()`, `MCAN_GetEsr()`, `MCAN_GetTsr()`, `MCAN_GetHalErrorCode()`, and
  `COM_SendSysInfoCommon()` for these new SysInfo types (request `0x550 [node, type]`, answer `0x560`):

  | type | value |
  |---|---|
  | 0x10 | live ESR (REC<<24, TEC<<16, LEC<<4, BOFF<<2, EPVF<<1, EWGF) |
  | 0x11 | live TSR (TME2..0 = bits 28..26) |
  | 0x12 | ring max << 16 \| ring level |
  | 0x13 | ring drops |
  | 0x14 | TXOK count |
  | 0x15 | consecutive failed recoveries << 16 \| total recoveries |
  | 0x16 | ms since last TXOK (or since nothing was pending) |
  | 0x17 | bus-off edges << 24 \| error-passive edges << 16 \| REC max << 8 \| TEC max |
  | 0x18 | ESR at last recovery |
  | 0x19 | HAL error bits EWG..CRC seen since start (rev. 2; `ErrorCode` itself is now consumed by the monitor) |
  | 0x1A | pointer repairs << 24 \| HAL state repairs << 16 \| error reports (16 bit) (rev. 2) |

- Test: `VarDefs/tests/test_mcan.py` (host gcc, simulated bxCAN with rc_w1/ABRQ/INRQ semantics, built with and
  without self-reset). It covers normal traffic, an idle node, a healed bus-off, a persistent stall, a full ring and
  tick wrap. Mutation checks: removing the recovery call or the drop counter makes it fail.

### ASW_ELECON, ASW_ELECON_D, ASW_ELECON_TEST
- `.ioc`: `CAN1.ABOM=ENABLE`, and `main.c` `AutoBusOff = ENABLE`, so a CubeMX regenerate keeps it. (MCAN forces it
  anyway.)
- `APP.c`: `default:` branch of the SysInfo switch calls `COM_SendSysInfoCommon()`.

### Other nodes
TECHM, IOBOARD_D/U, RECU and EVSE already have ABOM. They get the supervision, the counters and the 0x620 event on
their next build without changes. Only the SysInfo `default:` line is missing in their `APP.c` (one line each; EVSE
has no SysInfo handler). The IOBOARDs' second CAN (hcan2, ABOM = 0) is not used by MCAN.

### Behaviour changes / risks
- With ABOM on, a node with a *persistent* fault rejoins the bus every ~6 ms and transmits up to 32 erroneous frames
  each time (error frames on the bus). This is the normal CAN behaviour of the other five nodes already.
- A recovery loses up to 3 aborted frames. Up to 100 queued (≤ ~1 s old) frames go out after it.
- RAM +~90 B static, no heap, no extra interrupts. `RecoverTx()` busy-waits at most 100 000 loops (well below 1 ms).
- Verified: ARM gcc compile of MCAN.c/COM.c/LOG.c for ELECON, ELECON_D, TECHM, IOBOARD_D, RECU, EVSE (L4 + F4,
  no new warnings), APP.c/main.c of the three changed nodes, host tests (new + existing VARS2 tests) pass.
  **Not verified:** full link/build in CubeIDE, behaviour on hardware (section 6).
