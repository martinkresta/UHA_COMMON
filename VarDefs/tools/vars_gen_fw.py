"""
vars_gen_fw.py - VARS2 firmware C code generator (spec section 4).

Generates from uha_vars.yaml:
    generated/uha_vars_gen.h   eVarId, enums, group structs, sVars, externs, counts
    generated/uha_vars_gen.c   Vars2, VarMeta[], VarIdToIndex[]

generated/VARS2_types.h is hand-written (sVarMeta, eVarType, VMF_*).

The generator does not validate (run the validator first, tools/gen_all.py),
but refuses to emit code for values it cannot render (missing int
period/timeout, unknown producer, ...).

Usage:
    python tools/vars_gen_fw.py <yaml> [--inc-dir DIR] [--src-dir DIR] [-o <outdir>]

Default output: uha_vars_gen.h -> UHA_COMMON/Inc, uha_vars_gen.c -> UHA_COMMON/Src
(both folders are compiled into every ASW project). -o/--outdir overrides both
with a single directory (handy for the PC compile check / scratch use).
"""

import argparse
import math
import os
import sys
from typing import List

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vars_model import Model, VarsYamlError, load_model  # noqa: E402

HDR_NAME = "uha_vars_gen.h"
SRC_NAME = "uha_vars_gen.c"

C_TYPE = {"float": "float", "int32": "int32_t", "uint32": "uint32_t", "bool": "uint32_t", "enum": "uint32_t"}
VT = {"float": "VT_FLOAT", "int32": "VT_INT32", "uint32": "VT_UINT32", "bool": "VT_BOOL", "enum": "VT_ENUM"}
U16_MAX = 0xFFFF


class GenError(Exception):
    pass


def _is_int(x) -> bool:
    return isinstance(x, int) and not isinstance(x, bool)


def _ascii(text) -> str:
    """Comment-safe ASCII text."""
    s = str(text).replace("°C", "degC").replace("°", "deg")
    s = s.encode("ascii", "replace").decode("ascii")
    return s.replace("*/", "* /").replace("/*", "/ *")


def _c_float(x) -> str:
    f = float(x)
    if not math.isfinite(f):
        raise GenError(f"legacy_scale {x!r} is not a finite number")
    s = repr(f)                      # shortest exact round-trip: '0.1', '10.0', '1e-05'
    if "." not in s and "e" not in s:
        s += ".0"
    return s + "f"


def _header_comment(model: Model) -> List[str]:
    return [
        f"/* AUTO-GENERATED from uha_vars.yaml - DO NOT EDIT",
        f" * YAML version: {model.version}",
        f" * regenerate with tools/gen_all.py",
        f" */",
    ]


def _check(model: Model):
    """Fail on anything that would produce garbage C."""
    errs = []
    nodes = model.nodes
    for g in model.groups:
        if not g.vars:
            errs.append(f"group '{g.key}': no vars (empty C struct)")
    for v in model.values:
        where = f"{v.name} (id {v.id}, line {v.line})"
        if not (0 <= v.id < U16_MAX):
            errs.append(f"{where}: id out of uint16 range")
        if v.type not in C_TYPE:
            errs.append(f"{where}: unknown type {v.type!r}")
        if v.type == "enum" and v.enum not in model.enums:
            errs.append(f"{where}: unknown enum {v.enum!r}")
        if not _is_int(v.period_ms) or not (0 <= v.period_ms <= U16_MAX):
            errs.append(f"{where}: period_ms missing or not an int 0..65535 ({v.period_ms!r})")
        if not _is_int(v.timeout_ms) or not (0 <= v.timeout_ms <= U16_MAX):
            errs.append(f"{where}: timeout_ms missing or not an int 0..65535 ({v.timeout_ms!r})")
        if v.producer not in nodes or v.producer == "RPI":
            errs.append(f"{where}: producer {v.producer!r} is not a producing node")
        if v.legacy is not None and not (isinstance(v.legacy, str) and v.legacy.isidentifier()):
            errs.append(f"{where}: legacy {v.legacy!r} is not a C identifier")
        try:
            _c_float(v.legacy_scale)
        except (TypeError, ValueError, GenError):
            errs.append(f"{where}: legacy_scale {v.legacy_scale!r} is not a number")
        if not v.base.c_field:
            errs.append(f"{where}: no C field name")
    ids = [v.id for v in model.values]
    if len(ids) != len(set(ids)):
        errs.append("duplicate ids")
    for ename, emap in model.enums.items():
        if not isinstance(emap, dict) or not all(_is_int(k) for k in emap):
            errs.append(f"enum {ename}: must map int -> name")
    if errs:
        raise GenError("cannot generate FW code:\n  " + "\n  ".join(errs))


def _field_comment(d) -> str:
    ids = f"id {d.id}..{d.id + d.n - 1}" if d.is_array else f"id {d.id}"
    parts = [ids]
    if d.unit:
        parts.append(_ascii(d.unit))
    if d.type == "bool":
        parts.append("bool")
    elif d.type == "enum":
        parts.append(f"enum e{d.enum}")
    if not d.on_can:
        parts.append("not on CAN")
    return "/* " + ", ".join(parts) + " */"


def gen_header(model: Model) -> str:
    vals = model.values
    L = _header_comment(model) + [
        "",
        "#ifndef UHA_VARS_GEN_H_",
        "#define UHA_VARS_GEN_H_",
        "",
        "#include <stdint.h>",
        '#include "VARS2_types.h"',
        "",
        f"#define VARS_COUNT         {len(vals)}",
        f"#define VAR_MAX_ID         {model.max_id}",
        f"#define VARS_ON_CAN_COUNT  {sum(1 for v in vals if v.on_can)}",
        "",
        "/* ---- variable ids ---- */",
        "typedef enum",
        "{",
    ]
    w = max(len(v.vid) for v in vals)
    for v in vals:
        unit = f", {_ascii(v.unit)}" if v.unit else ""
        L.append(f"  {v.vid:<{w}} = {v.id:>3},  /* {v.group}{unit} */")
    L += ["} eVarId;", ""]

    if model.enums:
        L.append("/* ---- enums ---- */")
        for ename, emap in model.enums.items():
            L += ["typedef enum", "{"]
            items = sorted(emap.items())
            ew = max(len(f"{ename}_{n}") for _, n in items)
            for k, n in items:
                L.append(f"  {ename + '_' + str(n):<{ew}} = {k},")
            L += [f"}} e{ename};", ""]

    L.append("/* ---- group structs (4-byte fields only) ---- */")
    for g in model.groups:
        L += ["typedef struct", "{"]
        decls = []
        for d in g.vars:
            arr = f"[{d.n}]" if d.is_array else ""
            decls.append((C_TYPE[d.type], f"{d.c_field}{arr};", _field_comment(d)))
        tw = max(len(t) for t, _, _ in decls)
        fw = max(len(f) for _, f, _ in decls)
        for t, f, c in decls:
            L.append(f"  {t:<{tw}} {f:<{fw}} {c}")
        L += [f"}} {g.c_struct};", ""]

    L += ["typedef struct", "{"]
    sw = max(len(g.c_struct) for g in model.groups)
    for g in model.groups:
        L.append(f"  {g.c_struct:<{sw}} {g.c_member};")
    L += [
        "} sVars;",
        "",
        "extern sVars Vars2;",
        "extern const sVarMeta VarMeta[];        /* VARS_COUNT entries, sorted by id */",
        "extern const uint16_t VarIdToIndex[];   /* VAR_MAX_ID + 1 entries, VAR_IDX_UNKNOWN = unknown id */",
        "",
        "#endif /* UHA_VARS_GEN_H_ */",
        "",
    ]
    return "\n".join(L)


def gen_source(model: Model) -> str:
    vals = model.values
    rows = []
    for v in vals:
        rows.append([
            f"&Vars2.{v.c_ref},",
            f"{_c_float(v.legacy_scale)},",
            f"{v.id},",
            f"{v.period_ms},",
            f"{v.timeout_ms},",
            f"{v.legacy if v.legacy else 'VAR_LEGACY_NONE'},",
            f"{VT[v.type]},",
            f"NODEID_{v.producer},",
            "VMF_ON_CAN" if v.on_can else "0",
        ])
    head = ["/* pValue", "scale", "id", "period", "tmout", "legacyId", "type", "producer", "flags */"]
    widths = [max(len(r[i]) for r in rows + [head]) for i in range(len(head))]

    def fmt(cells):
        return " ".join(f"{c:<{widths[i]}}" for i, c in enumerate(cells)).rstrip()

    L = _header_comment(model) + [
        "",
        '#include "uha_vars_gen.h"',
        '#include "VARS.h"   /* legacy VAR_* ids */',
        '#include "MCAN.h"   /* NODEID_* */',
        "",
        "sVars Vars2;",
        "",
        "const sVarMeta VarMeta[VARS_COUNT] =",
        "{",
        "   " + fmt(head),
    ]
    for r in rows:
        L.append("  { " + fmt(r) + " },")
    L += ["};", ""]

    index = {v.id: i for i, v in enumerate(vals)}
    L += ["const uint16_t VarIdToIndex[VAR_MAX_ID + 1] =", "{"]
    per_line = 10
    for start in range(0, model.max_id + 1, per_line):
        cells = []
        for vid in range(start, min(start + per_line, model.max_id + 1)):
            cell = str(index[vid]) if vid in index else "VAR_IDX_UNKNOWN"
            cells.append(f" {cell:>15}" + ("," if vid < model.max_id else ""))
        L.append(f"  /* {start:>3} */" + "".join(cells))
    L += ["};", ""]
    return "\n".join(L)


def _write(path: str, text: str) -> bool:
    data = text.encode("ascii")  # raises on any non-ASCII slip
    if os.path.exists(path):
        with open(path, "rb") as f:
            if f.read() == data:
                return False
    with open(path, "wb") as f:
        f.write(data)
    return True


def generate(model: Model, inc_dir: str, src_dir: str) -> List[str]:
    """Write uha_vars_gen.h into inc_dir and uha_vars_gen.c into src_dir;
    returns the paths of files actually rewritten."""
    _check(model)
    os.makedirs(inc_dir, exist_ok=True)
    os.makedirs(src_dir, exist_ok=True)
    written = []
    for name, outdir, text in (
        (HDR_NAME, inc_dir, gen_header(model)),
        (SRC_NAME, src_dir, gen_source(model)),
    ):
        p = os.path.join(outdir, name)
        if _write(p, text):
            written.append(p)
    return written


def main(argv=None) -> int:
    tools_dir = os.path.dirname(os.path.abspath(__file__))
    default_inc = os.path.normpath(os.path.join(tools_dir, "..", "..", "Inc"))
    default_src = os.path.normpath(os.path.join(tools_dir, "..", "..", "Src"))
    ap = argparse.ArgumentParser(description="VARS2 FW C code generator")
    ap.add_argument("yaml")
    ap.add_argument("--inc-dir", default=default_inc, help="output dir for uha_vars_gen.h")
    ap.add_argument("--src-dir", default=default_src, help="output dir for uha_vars_gen.c")
    ap.add_argument("-o", "--outdir", help="override: write both files into this single dir")
    a = ap.parse_args(argv)
    inc_dir = a.outdir if a.outdir else a.inc_dir
    src_dir = a.outdir if a.outdir else a.src_dir
    try:
        model = load_model(a.yaml)
        from vars_validate import validate   # never write from an invalid YAML
        errs = [i for i in validate(model) if i.severity == "E"]
        if errs:
            for i in errs:
                print(i.format(a.yaml), file=sys.stderr)
            print(f"ERROR: {len(errs)} validation errors, nothing generated (run tools/gen_all.py)",
                  file=sys.stderr)
            return 1
        written = generate(model, inc_dir, src_dir)
    except (VarsYamlError, GenError) as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1
    for name, outdir in ((HDR_NAME, inc_dir), (SRC_NAME, src_dir)):
        p = os.path.join(outdir, name)
        print(f"{'written  ' if p in written else 'unchanged'} {p}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
