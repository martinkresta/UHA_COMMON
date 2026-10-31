# Task: VARS2 step 0 — YAML loader, validator and FW code generator

## 1. Context

UHA is a home automation system for an off-grid house: STM32 nodes on a CAN
bus, firmware in C (STM32CubeIDE, no RTOS). Workspace `C:\Martin\GIT\UHA_FW`;
read `UHA_FW/CLAUDE.md` and `UHA_COMMON/CLAUDE.md` first.

The shared variable protocol (VARS) is being replaced by **VARS2**: 4-byte
values (float/int32/uint32), a new CAN frame, and one definition file as the
**single source of truth** from which code is generated.

Everything you need is in `UHA_COMMON/VarDefs/`:

| File | Content |
|------|---------|
| `VARS2_spec.md` | **The specification — read it fully.** Frame, YAML schema, validator rules, generated code, migration plan |
| `uha_vars.yaml` | The definition (257 values, reviewed by Martin) |
| `vars_dependency_map.md` | Who publishes / reads what today |

Reference (read-only): `UHA_COMMON/Inc/VARS.h` (old IDs), `UHA_COMMON/Src/VARS.c`,
`UHA_COMMON/Src/ELM.c`, `UHA_COMMON/Inc/MCAN.h` (`NODEID_*`).

This task is **migration step 0** only: tooling that reads the YAML, checks
it and generates the FW files. No firmware module is changed.

---

## 2. Hard constraints

- Work only in `UHA_COMMON/VarDefs/` (new subfolders allowed).
- **Do not modify** any existing file in `UHA_COMMON/Inc`, `UHA_COMMON/Src`, any
  ASW/BSW project, or `uha_vars.yaml` (if the YAML has a problem, report it).
- **Generated C files go to `UHA_COMMON/VarDefs/generated/`, NOT to
  `UHA_COMMON/Src` or `Inc`.** Those folders are linked into every ASW project;
  a new `.c` there would be compiled into all nodes. Moving them there is a
  later step.
- Python 3 + PyYAML only (no other dependencies). Scripts must run on Windows.
- Generated C: C99, must compile with `gcc -std=c99 -Wall -Wextra -c` on the
  PC using a small stub header for target-only symbols.
- Follow `VARS2_spec.md`. Where it is unclear or you would deviate, ask.

---

## 3. What to build

```
UHA_COMMON/VarDefs/
  tools/
    vars_model.py        # shared loader: YAML -> validated-free model (expanded arrays, defaults applied)
    vars_validate.py     # CLI: validation rules, exit code != 0 on error
    vars_gen_fw.py       # CLI: generates generated/uha_vars_gen.h + .c
    gen_all.py           # runs validate, then generate (one command for Martin)
  generated/
    uha_vars_gen.h
    uha_vars_gen.c
    VARS2_types.h        # hand-written: sVarMeta, eVarType (VT_FLOAT ...), flags — needed by gen files
  tests/                 # small YAML fixtures that must pass / fail the validator
  README.md              # how to run, what is generated
```

### 3.1 Model (`vars_model.py`)

- Load YAML; apply `defaults`; group `producer` inheritance; `timeout_ms: auto`
  = 3 × `period_ms`; expand `count` arrays to individual values (id, index,
  legacy name `VAR_BMS1_CELL{1..16}_MV` → `VAR_BMS1_CELL1_MV` …).
- Provide names: C field name (LEMON style, spec §4: CamelCase + unit suffix,
  group prefix dropped), struct name `sVars<Group>`, member name `<Group>`
  (CamelCase), ID name `VID_<NAME>`.

### 3.2 Validator (`vars_validate.py`)

All rules from spec §3, plus:
- every `legacy` name exists in `UHA_COMMON/Inc/VARS.h` **with the same ID**;
- every value has a producer that exists in `nodes`;
- `period_ms` present for values with a producer;
- generated C names are unique (after CamelCase conversion);
- ELM rule (`id + 10` power pair) — see `ELM.c`;
- optional `--baseline <old yaml>`: compatibility check (no ID/type change,
  no reuse of a removed ID).

Output: clear list of errors / warnings with YAML location; summary counts.

### 3.3 FW generator (`vars_gen_fw.py`)

Exactly as spec §4: `eVarId`, enums, one struct per group (4-byte types only;
enum and bool stored as `uint32_t`), `sVars` with all groups, `extern sVars Vars2`,
`VARS_COUNT`, `VAR_MAX_ID`; `.c` with `Vars2`, `const sVarMeta VarMeta[]`
(id, type, pValue, producer, period, timeout, legacy_scale, legacy id, on_can)
and `const uint16_t VarIdToIndex[VAR_MAX_ID + 1]` (0xFFFF = unknown).
Header comment: "AUTO-GENERATED from uha_vars.yaml – DO NOT EDIT" + YAML version.
Output must be deterministic (same YAML → byte-identical files).

---

## 4. How to work — use subagents

1. **Phase 0 — Git** (repo `UHA_COMMON`): `git status`; if there are
   uncommitted changes, ask Martin. Create branch `vars2-step0`.
2. **Phase 1 — Study and plan (STOP for review).** Read the spec, YAML and
   reference files. Write a short plan: model API, list of validator rules,
   sample of the generated header for 2 groups, open questions. **Stop and
   wait for Martin's approval.**
3. **Phase 2 — Implement:**
   - You write `vars_model.py` first (the shared model).
   - Then run **two subagents in parallel**: (a) validator + test fixtures,
     (b) FW generator + `VARS2_types.h` + compile check. Give each the spec,
     the model API and the constraints above.
   - Then run a **third, independent reviewer subagent** that did not take
     part in writing the code: it checks the generated files and validator
     against `VARS2_spec.md` and `uha_vars.yaml` (spot-check at least 20
     values incl. arrays, enums, legacy_scale, on_can, ELM pairs) and reports
     discrepancies. Fix what it finds.
4. Run `gen_all.py` on the real `uha_vars.yaml`; compile the generated code
   with gcc. Commit in small logical commits on `vars2-step0`. Do not merge.

## 5. Report to Martin at the end

- Validator result on `uha_vars.yaml` (errors / warnings).
- Generated file sizes, `VARS_COUNT`, `VAR_MAX_ID`, estimated RAM/flash
  (VarMeta size, Vars2 size, `VarIdToIndex` size).
- Reviewer findings and what was fixed.
- Anything in the spec you had to interpret.
