# VARS2 step 2 — working notes (ELECON → RPi VARS2 tunnel)

Task: `UHA_COMMON/tasks/elecon_vars2_tunel.md`, branch `feature/elecon_vars2_tunel`
(from `main`; step 1 `vars2-step1` was already merged in every repo).
All repos were clean at start (the `.cproject` of ELECON_D and
`ASW_IOBOARD_D/Deployment/v001.00/*` mentioned in the brief showed no
changes, nothing to protect).

## Design

| Req | Implementation |
|---|---|
| 1, 2 tunnel of RX / own TX frames | `VARS2_SetFrameHook(hook)` in `VARS2.c`; hook called from `VARS2_ProcessRxFrame()` (RX, before the id lookup, so unknown ids are forwarded too) and from `VARS2_SendLegacy()` (TX, cobId `0x180+node`). ELECON registers `TunnelVars2Frame()` in `SCOM_Init()` → `RSP_Send(id_hi id_lo + 8 bytes)`. Default hook = NULL, other nodes unchanged. |
| 3 local non-CAN variables | `VARS2_UpdateLocal_10ms(cb)`: rows with producer == this node, no `VMF_ON_CAN`, `PeriodMs > 0`; per-row 16-bit send tick (`mLocalTick`), first sends staggered by 10 ms per row, max 4 frames per call. Frame built by the same static `BuildLegacyFrame()` as `VARS2_SendLegacy()` (no duplicated conversion). Called from `SCOM_Update_10ms()`. |
| 4 old traffic | scan list / 0x620 / RX handling untouched. |
| 5 throughput | `RPISERP.c`: own TX ring of `RSP_TX_FIFO_SIZE` = 64 frames (was `CB` with 10). `RSP_Send()` and the consumer `TryStartTx()` run with interrupts disabled (PRIMASK saved/restored). `RSP_Send()` starts the transmitter at once if idle; the next frame is chained from the USART TC interrupt (`mTxBusy` is now released only there, the DMA TC IRQ just disables the channel). `RSP_TransmitFromFifo()` stays as a periodic kick. FIFO full → new frame dropped, `gRspTxOverflow++` (debug variable, also `RSP_GetTxOverflowCount()`); `gRspTxFrames` counts frames sent. |
| 6a | `BuildLegacyFrame()` sets `mState[idx].Valid` / `RxTick` for the producer's own value. |
| 6b | `VARS2_ProcessRxFrame(cobId, dlc, rtr, data)` rejects DLC != 8 and RTR (returns 0); `COM_GetRxMessage()` calls it for every `0x18x` frame, rejected frames are dropped (consumed, not returned to the application). |

Why `CB_*` was not reused for the TX FIFO: `CB_Put()` is not interrupt-safe
(only `CB_Get` masks IRQs) and silently overwrites the oldest item when full
(no overflow information); `circbuf.c` is outside the allowed change list.

## Spec §7 interpretation

- `ELECON_TEST` / any node 4 produces `0x184`; the tunnel packet id for own
  frames is `0x180 + mThisNodeId`, which is `0x184` on ELECON.
- 108 ELECON-local `on_can: false` rows exist (spec estimated ~44 frames/s;
  actual sum of 1/period ≈ 42/s, matches).
- Unknown VARS2 ids received from CAN are tunnelled as well (spec: "every
  VARS2 frame received").
- DLC/RTR-rejected frames are not tunnelled.

## Tests (PC, gcc)

- `VarDefs/tests/test_vars2.py` — extended (hook, DLC/RTR, own state,
  local publishing periods / conversion / burst, other node publishes none).
- `VarDefs/tests/test_tunnel.py` — new: compiles the real `RPISERP.c` +
  `SCOM.c` (copied to a temp dir so `tests/stub_rsp/main.h` wins) with a
  simulated UART/DMA/IRQ line (115200 Bd byte budget): tunnel frame bytes and
  checksum, rejected frames, `0x184` own frames, 60 s run (local periods,
  old `0x221` still sent, nothing extra on CAN), FIFO overflow count (1 in
  UART + 64 queued, rest counted, order kept), 500 frames/s offered for
  10 s without a drop, PRIMASK balanced.

## Build results (headless)

| Node | Version | CRC | Flash (text+data) before → after | RAM (data+bss) before → after |
|---|---|---|---|---|
| ELECON | v001.12 | 0x6842378F | 90488 → 91384 | 26104 → 27528 |
| ELECON_D | v001.17 | 0x166C51B3 | 106692 → 106916 | 23496 → 23496 |
| IOBOARD_D | v001.02 | 0x6E226BD7 | 60800 → 61020 | 43032 → 43032 |
| IOBOARD_U | v001.02 | 0x05EB25CC | 45268 → 45400 | 43200 → 43200 |
| TECHM | v001.04 | 0x89853F65 | 58504 → 58640 | 11120 → 11128 |
| RECU_NUCLEO | v001.02 | 0x3955D4B5 | 62584 → 62804 | 9944 → 9944 |

(ELECON RAM +1424 B = TX FIFO 896 B + local ticks 514 B + counters.)
Other nodes: `UpdateLocal` and its tick array are removed by `--gc-sections`.

## Desk test (CAN side only)

Not performed here — needs the physical ELECON_TEST board on the desk bus
(Martin). Checklist: ELECON build still sends old `0x110+4` and `0x184`
frames on CAN as in step 1; nothing else changes on CAN (the tunnel and
local rows go to the UART only).
