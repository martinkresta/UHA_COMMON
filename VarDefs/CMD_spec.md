# UHA command channel — specification (step 3)

Shared by the FW brief (`UHA_COMMON/tasks/cmd_channel_fw.md`) and the RPi brief
(`UHARPI/tasks/control_web_api.md`). Single source of truth for the format; change it here first.

## Principles (agreed 2026-11-17)

- Commands are **requests** to the node that owns the function. The node decides; local safety always wins.
- **Feedback = the VARS2 state** of that function (step 2, `send: on_change`). No ACK/NACK now; byte `seq` is
  reserved so an ACK can be added later without changing the format.
- **The target module clamps** values silently to its valid range (in its setter, for every source of the
  request, not only CAN). Unknown command ids are ignored.
- ELECON **only forwards** RPi commands; it does not check ids or ranges. It must never let the RPi choose a CAN id.
- Defined in `uha_vars.yaml` (section `commands:`), generated into FW and read by UhaGatePy/UI.
- Not in this step: re-sending setpoints after a node restart (may be added later, RPi only), roles/rights.

## 1. Web → UhaGatePy

HTTPS on `ctrl.uha.mkresta.cz` with mandatory client certificate (nginx), proxied to UhaGatePy on 127.0.0.1.

- `GET /api/commands` → list from the YAML: id, name, target, type, min, max, state, desc.
- `POST /api/cmd` with JSON `{"cmd": "<name>", "value": <number>}` → 200 `{"ok": true, "seq": n}` when sent to the
  serial link; 400 unknown command / bad value type; 503 serial link down; 429 over the gateway rate limit.
- Identity: nginx sets header `X-Client-DN` from `$ssl_client_s_dn`; the gateway takes the CN as the user name and
  logs every command (journal + DB table `commands`). Requests without the header are refused (403).

## 2. RPi → ELECON: RPISERP packet `0x52 CMD_RPI_COMMAND`

Normal RPISERP framing (`0x7F 0xAA | len | id_hi id_lo | data | checksum`, `len = 2 + 8 = 10`). Data, 8 bytes:

| Byte | Content |
|---|---|
| 0 | target node id (1..15, `nodes:` in the YAML) |
| 1–2 | command id, u16 **little endian** |
| 3–6 | value, 4 bytes little endian: float32 / int32 / uint32 by the command's `type` (bool, enum as int32) |
| 7 | seq (0..255, increments per command, wraps; reserved for a future ACK) |

## 3. ELECON forwarding (`ASW_ELECON/Core/Src/SCOM.c`, `ProcessMessage()`)

For `id == 0x52`:
1. data length must be exactly 8, else drop (count).
2. rate limit: at most **10 per second and 20 per minute** (two counters, reset every 1 s / 60 s by `HAL_GetTick()`);
   over the limit → drop (count).
3. `target = data[0] & 0x0F`. If `target == own node (4)` → handle locally via `CMD_ProcessFrame()` (section 4).
   Otherwise send CAN frame **id `0x200 + target`** (fixed base, never anything from the packet), DLC 8, data
   `[cmd_lo, cmd_hi, v0, v1, v2, v3, seq, 0]`.
4. Every drop: one `LOG` event (rate-limited) so it shows in the RPi `log.txt`.

No other RPISERP id may ever reach the CAN bus as a raw frame. (Today only `0x777` heartbeat and `0x51` RTC are
handled; `0x50` stays unimplemented.)

## 4. CAN command frame and node side

- CAN id **`0x200 + target node`** (like CANopen RPDO1: the node *receives* on it; VARS2 = `0x180 + producer`
  like TPDO1). Range `0x200–0x20F` is free today. Note: the existing `COM_SendMessage(cobid, …)` adds the
  **sender** node id — do not use it for commands.
- Data 8 bytes: `[cmd_lo, cmd_hi, v0, v1, v2, v3, seq, reserved=0]`, DLC must be 8.
- Common module `UHA_COMMON/Src/CMD.c` (`CMD.h`): `CMD_ProcessFrame(const uint8_t* data)` looks the id up in the
  generated table, ignores unknown ids and ids whose `target` is not this node, converts the 4 value bytes by
  `type` and calls the node's `APP_OnCommand(eCmdId id, float value)` (one function per node, a `switch` that
  calls the existing module APIs). No range checks in `CMD.c`.
- Hook: `COM_GetRxMessage()` (`UHA_COMMON/Src/COM.c`) consumes VARS2 frames itself; consume `0x200 + own node`
  the same way and pass it to `CMD_ProcessFrame()`. Check the MCAN hardware filter lets `0x200 + own node` through.

## 5. YAML `commands:` section

```yaml
commands:
  - id: 201
    name: CMD_LIGHT_LOFT_WEST        # unique, CMD_ prefix
    target: IOBOARD_U                # node from nodes:, not RPI
    type: uint32                     # float | int32 | uint32 | bool | enum (+ enum:)
    min: 0
    max: 100
    unit: "%"
    state: ST_LIGHT_LOFT_WEST        # VARS2 state variable produced by the target (UI shows it as feedback)
    desc: Loft west light, 0 = off, 100 = on
```

- **Command id ranges = node × 100**: IOBOARD_D 100–199, IOBOARD_U 200–299, TECHM 300–399, ELECON 400–499,
  RECU 500–599, ELECON_D 700–799. Ids are never reused.
- Generator → `UHA_COMMON/Inc/uha_cmds_gen.h` / `Src/uha_cmds_gen.c`: `eCmdId` (`CMDID_<name>`), table
  `{id, target, type}`, and macros `CMD_<NAME>_MIN` / `CMD_<NAME>_MAX` that the module setters use for clamping
  (one source of truth for UI and FW limits).
- Validator: unique id/name, id in the target's range, target in `nodes` and not RPI, known type, `min <= max`,
  `state` exists and is produced by the target.

## 6. Pilot commands (step 3a)

One command per light of IOBOARD_U (the lights published in `States.IoboardU` in step 2a), `uint32` 0..100,
`state` = the matching `ST_LIGHT_*`. UI sends 0 (off) or 100 (on); the lights are not dimmed today.
Handler: `LC_RemoteControl(lightId, intensity)` in `ASW_IOBOARD_U` (clamp to `CMD_..._MIN/MAX` there or in a
small setter wrapper).
