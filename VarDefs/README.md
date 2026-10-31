# VarDefs — VARS2 definitions and tooling

`uha_vars.yaml` is the single source of truth for the shared UHA variables (VARS2).
The tools in `tools/` check it and generate the firmware tables. Protocol, YAML schema
and migration plan: [VARS2_spec.md](VARS2_spec.md).

Requirements: Python 3 + PyYAML (`pip install pyyaml`). gcc only for the PC compile check.

## One command

```
python tools/gen_all.py
```

Validates `uha_vars.yaml` and, if there are no errors, regenerates `generated/uha_vars_gen.h/.c`
(files are rewritten only when their content changes). Exit code: 0 OK, 1 validation errors
(nothing generated), 2 YAML cannot be read.

Options: `--baseline <old released yaml>` (compatibility check), `--src-root C:\Martin\GIT\UHA_FW`
(cross-check `ELM_AddMeter()` calls in the ASW sources), `-o <dir>` (output folder).

## Files

| Path | What |
|------|------|
| `uha_vars.yaml` | Variable definitions (edit this) |
| `tools/vars_model.py` | Shared loader: defaults, producer inheritance, `timeout_ms: auto` = 3 × period, array expansion, C names |
| `tools/vars_validate.py` | Validator (CLI + `validate()`); errors/warnings with YAML line numbers |
| `tools/vars_gen_fw.py` | FW generator (CLI + `generate()`) |
| `tools/gen_all.py` | Validate + generate |
| `generated/VARS2_types.h` | Hand-written: `eVarType`, `sVarMeta`, flags |
| `generated/uha_vars_gen.h/.c` | **Generated – do not edit** |
| `tests/run_validator_tests.py` | Runs the validator on `tests/fixtures/` (`pass_*` / `fail_*`) |
| `tests/compile_check.py` | Compiles the generated C with `gcc -std=c99 -Wall -Wextra -pedantic` on the PC (stubs in `tests/stub/`) and runs table consistency checks |

## What is generated

- `eVarId` — `VID_<NAME> = <id>`; arrays expanded as `VID_<NAME>_1 .. _<count>`.
- `e<Enum>` — `<Enum>_<Name> = <value>` for each YAML enum.
- One struct per group, `sVars<Group>`, 4-byte fields only (float / int32_t / uint32_t;
  bool and enum stored as uint32_t). Field names: CamelCase, unit suffix `_<Unit>` when the
  last name token matches `unit`, the group's common name prefix dropped
  (`BMS1_CELL_TEMP_C` in `bms1` → `CellTemp_C[16]`).
- `sVars` with all groups, `extern sVars Vars2`, `VARS_COUNT`, `VAR_MAX_ID`, `VARS_ON_CAN_COUNT`.
- `.c`: `Vars2`, `const sVarMeta VarMeta[]` (sorted by id: pointer, legacy scale, id, period,
  timeout, legacy `VAR_*` id, type, producer `NODEID_*`, flags) and
  `const uint16_t VarIdToIndex[VAR_MAX_ID + 1]` (`VAR_IDX_UNKNOWN` = 0xFFFF).

The generated `.c` includes `VARS.h` and `MCAN.h` from `UHA_COMMON/Inc`.
It stays in `generated/` until a later migration step moves it to `UHA_COMMON/Src`/`Inc`
(those folders are compiled into every node).

## Rules for changing the YAML

- Never change the `id` or `type` of an existing variable; never reuse a removed id.
  Run with `--baseline` against the last released YAML to check.
- Every value needs a producer (group or own) and `period_ms`.
- Electricity meters: `CONS_x_WH` at id N needs `POW_x_W` at id N + 10 (ELM writes it there).
