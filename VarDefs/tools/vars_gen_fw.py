"""
vars_gen_fw.py - VARS2 firmware C code generator (spec section 4).

Generates from uha_vars.yaml:
    uha_vars_gen.h   eVarId, enums, group structs ({value, valid} fields), sVars, externs, counts
    uha_vars_gen.c   Vars2, VarMeta[], VarIdToIndex[], struct size checks

VARS2_types.h is hand-written (sVarF32/sVarI32/sVarU32, sVarMeta, eVarType, VMF_*).

Struct types: one per top-level group, or one per shared `struct`; a container
gets one type per subgroup (sVars<Group|struct><Subgroup>) and its own type
with the subgroups as members. Every field is an 8-byte {value, valid} item
and every struct is a plain array of them (checked in the .c), so
VARS2_SetGroupValid() can walk any group or container by its size.

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
import os
import sys
from typing import List

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vars_model import Model, VarsYamlError, load_model  # noqa: E402

HDR_NAME = "uha_vars_gen.h"
SRC_NAME = "uha_vars_gen.c"

C_TYPE = {"float": "sVarF32", "int32": "sVarI32", "uint32": "sVarU32", "bool": "sVarI32", "enum": "sVarI32"}
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
    for g in model.leaf_groups:
        if not g.vars:
            errs.append(f"group '{g.path}': no vars (empty C struct)")
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


def _field_comment(d, shared: bool) -> str:
    """Field comment; ids and CAN flag only for types used by one group."""
    parts = []
    if not shared:
        parts.append(f"id {d.id}..{d.id + d.n - 1}" if d.is_array else f"id {d.id}")
    if d.unit:
        parts.append(_ascii(d.unit))
    if d.type == "bool":
        parts.append("bool")
    elif d.type == "enum":
        parts.append(f"enum e{d.enum}")
    if not d.on_can and not shared:
        parts.append("not on CAN")
    return "/* " + ", ".join(parts) + " */" if parts else ""


def _struct_types(model: Model):
    """[(group, 'vars' | 'container')] in declaration order, one entry per C type
    (groups sharing a `struct` are emitted once, subgroup types before their container)."""
    out, seen = [], set()
    for g in model.groups:
        for sg in g.subgroups:
            if sg.c_struct not in seen:
                seen.add(sg.c_struct)
                out.append((sg, "vars"))
        if g.c_struct not in seen:
            seen.add(g.c_struct)
            out.append((g, "container" if g.is_container else "vars"))
    return out


def _is_shared(model: Model, g) -> bool:
    top = g.parent or g
    return isinstance(top.struct, str) and sum(1 for t in model.groups if t.struct == top.struct) > 1


def _group_comment(g) -> str:
    """ /* producer[, validity per field] */ """
    parts = []
    if g.producer is not None and not g.is_container:
        parts.append(str(g.producer))
    elif g.is_container:
        prods = sorted({str(s.producer) for s in g.subgroups})
        parts.append(prods[0] if len(prods) == 1 else "producer per subgroup")
    if not g.is_container and g.validity == "per_field":
        parts.append("validity per field")
    return "  /* " + ", ".join(parts) + " */" if parts else ""


def _n_items(g) -> int:
    if g.is_container:
        return sum(_n_items(sg) for sg in g.subgroups)
    return sum(d.n for d in g.vars)


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

    L.append("/* ---- group structs ({value, valid} fields, see VARS2_types.h) ---- */")
    for g, kind in _struct_types(model):
        shared = _is_shared(model, g)
        L += ["typedef struct", "{"]
        if kind == "container":
            sw = max(len(sg.c_struct) for sg in g.subgroups)
            mw = max(len(sg.c_member) for sg in g.subgroups) + 1
            for sg in g.subgroups:
                cmt = "" if shared else _group_comment(sg)
                L.append((f"  {sg.c_struct:<{sw}} {sg.c_member + ';':<{mw}}" + cmt).rstrip())
        else:
            decls = []
            for d in g.vars:
                arr = f"[{d.n}]" if d.is_array else ""
                decls.append((C_TYPE[d.type], f"{d.c_field}{arr};", _field_comment(d, shared)))
            tw = max(len(t) for t, _, _ in decls)
            fw = max(len(f) for _, f, _ in decls)
            for t, f, c in decls:
                L.append(f"  {t:<{tw}} {f:<{fw}} {c}".rstrip())
        L += [f"}} {g.c_struct};", ""]

    L += ["typedef struct", "{"]
    sw = max(len(g.c_struct) for g in model.groups)
    mw = max(len(g.c_member) for g in model.groups) + 1
    for g in model.groups:
        L.append(f"  {g.c_struct:<{sw}} {g.c_member + ';':<{mw}}" + _group_comment(g))
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
            f"{v.id},",
            f"{v.period_ms},",
            f"{v.timeout_ms},",
            f"{VT[v.type]},",
            f"NODEID_{v.producer},",
            "VMF_ON_CAN" if v.on_can else "0",
        ])
    head = ["/* pVar", "id", "period", "tmout", "type", "producer", "flags */"]
    widths = [max(len(r[i]) for r in rows + [head]) for i in range(len(head))]

    def fmt(cells):
        return " ".join(f"{c:<{widths[i]}}" for i, c in enumerate(cells)).rstrip()

    L = _header_comment(model) + [
        "",
        '#include "uha_vars_gen.h"',
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

    L += ["/* every struct is a plain array of 8-byte {value, valid} items (VARS2_SetGroupValid()) */"]
    tw = max(len(g.c_struct) for g, _ in _struct_types(model))
    for g, _ in _struct_types(model):
        L.append(f"typedef char VARS2_SIZE_{g.c_struct}[(sizeof({g.c_struct}){' ' * (tw - len(g.c_struct))} == "
                 f"{_n_items(g):>3}u * VAR_ITEM_SIZE) ? 1 : -1];")
    L.append(f"typedef char VARS2_SIZE_sVars[(sizeof(sVars) == {sum(_n_items(g) for g in model.groups)}u * VAR_ITEM_SIZE) ? 1 : -1];")
    L += [""]

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
