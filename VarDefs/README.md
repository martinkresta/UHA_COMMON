# VarDefs — VARS2 definitions and tooling

`uha_vars.yaml` is the single source of truth for the shared UHA variables (VARS2).
The tools in `tools/` check it and generate the firmware tables. Protocol, YAML schema
and migration plan: [VARS2_spec.md](VARS2_spec.md).

Requirements: Python 3 + PyYAML (`pip install pyyaml`). gcc only for the PC compile check.

## One command

```
python tools/gen_all.py
```

Validates `uha_vars.yaml` and, if there are no errors, regenerates `UHA_COMMON/Inc/uha_vars_gen.h`
and `UHA_COMMON/Src/uha_vars_gen.c` (files are rewritten only when their content changes). Those
two folders are linked into every ASW project, so the generated files are compiled into every
node. Exit code: 0 OK, 1 validation errors (nothing generated), 2 YAML cannot be read.

Options: `--baseline <old released yaml>` (compatibility check), `--src-root C:\Martin\GIT\UHA_FW`
(cross-check `ELM_AddMeter()` calls in the ASW sources), `--inc-dir`/`--src-dir` (override one of
the two output folders), `-o <dir>` (override: write both files into this single dir, e.g. for a
scratch compile check).

## Files

| Path | What |
|------|------|
| `uha_vars.yaml` | Variable definitions (edit this) |
| `tools/vars_model.py` | Shared loader: defaults, producer inheritance, `timeout_ms: auto` = 3 × period, array expansion, C names |
| `tools/vars_validate.py` | Validator (CLI + `validate()`); errors/warnings with YAML line numbers |
| `tools/vars_gen_fw.py` | FW generator (CLI + `generate()`) |
| `tools/gen_all.py` | Validate + generate |
| `../Inc/VARS2_types.h` | Hand-written: `sVarF32`/`sVarI32`/`sVarU32` (`{value, valid}`), `eVarType`, `sVarMeta`, flags |
| `../Inc/uha_vars_gen.h`, `../Src/uha_vars_gen.c` | **Generated – do not edit.** Compiled into every ASW project (new file: each project's `.project`/`.cproject` needs a linked-resource entry, same as the other `UHA_COMMON/Inc`/`Src` files) |
| `../Inc/VARS2.h`, `../Src/VARS2.c` | Hand-written VARS2 module (YAML-driven TX, RX into `Vars2`, timeout, `VARS2_SetGroupValid()`, `VarSet()`, `VARS2_SetById()`) — see `VARS2_spec.md` |
| `tests/run_validator_tests.py` | Runs the validator on `tests/fixtures/` (`pass_*` / `fail_*`) |
| `tests/compile_check.py` | Compiles the generated C with `gcc -std=c99 -Wall -Wextra -pedantic` on the PC (stubs in `tests/stub/`) and runs table consistency checks |
| `tests/test_vars2.py` | PC test of `VARS2.c`: RX, timeout, tick wrap, group validity, by-id API, TX periods / pacing, tunnel callback |
| `tests/test_com.py` | PC test of `COM.c` (stubs in `tests/stub_com/`): every node's own `on_can` variables at their YAML period, only `0x180+node`, a received `0x110+node` changes nothing |
| `tests/test_tunnel.py` | ELECON tunnel (`RPISERP.c`, `SCOM.c`) on top of VARS2 |

## What is generated

- `eVarId` — `VID_<NAME> = <id>`; arrays expanded as `VID_<NAME>_1 .. _<count>`.
- `e<Enum>` — `<Enum>_<Name> = <value>` for each YAML enum.
- Group structs: every field is an 8-byte `{value, valid}` item (`sVarF32` for float,
  `sVarI32` for int32 / bool / enum, `sVarU32` for uint32), field names from the YAML `field`.
  One type per top-level group (`sVars<Group>`), or one per shared `struct` (`sVars<struct>`);
  a container with `subgroups` gets one type per subgroup (`sVars<Group|struct><Subgroup>`) and
  its own type with the subgroups as members. Access: `Vars2.GlobalEnergy.Battery.Soc_pct.value`.
- `sVars` with all top-level groups, `extern sVars Vars2`, `VARS_COUNT`, `VAR_MAX_ID`,
  `VARS_ON_CAN_COUNT`.
- `.c`: `Vars2`, `const sVarMeta VarMeta[]` (sorted by id: pointer to the field, id, period, timeout,
  type, producer `NODEID_*`, flags), `const uint16_t VarIdToIndex[VAR_MAX_ID + 1]` (`VAR_IDX_UNKNOWN` =
  0xFFFF) and compile-time size checks: every struct is a plain array of 8-byte items, so
  `VARS2_SetGroupValid()` can walk it.

The generated `.c` includes `MCAN.h` (`NODEID_*`) from `UHA_COMMON/Inc`. `legacy` / `legacy_scale` in the
YAML are not used by the firmware (only by UhaGatePy for `uha.json`).

## Rules for changing the YAML

- Never change the `id` or `type` of an existing variable; never reuse a removed id.
  Run with `--baseline` against the last released YAML to check.
- Every group with `vars` has exactly one producer (on the group or its container, never on a
  variable); every value needs `period_ms` and a `field` with the unit suffix.
- Groups sharing a `struct` must have identical field lists.
- Electricity meters: `CONS_x_WH` at id N needs `POW_x_W` at id N + 10 (ELM writes it there).
