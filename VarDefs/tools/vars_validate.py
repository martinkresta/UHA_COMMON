"""
vars_validate.py - validator for uha_vars.yaml (VARS2), spec section 3.

CLI:
    python tools/vars_validate.py <yaml> [--vars-h PATH] [--baseline OLD.yaml]
                                         [--src-root DIR] [-q]

    Exit code: 0 = no errors, 1 = errors found, 2 = YAML unparseable / file missing.
    -q suppresses WARNING lines (errors and the summary are always printed).

Import:
    from vars_validate import validate
    issues = validate(model, vars_h=None, baseline=None, src_root=None)
    for i in issues: print(i.format(model.path))

    vars_h   None -> default <VarDefs>/../Inc/VARS.h (error if unreadable),
             False -> legacy-vs-VARS.h check skipped, str -> that file
    mcan_h   same for MCAN.h (node ids vs NODEID_*)
    baseline None, a path, or an already loaded Model
    src_root None or a directory scanned for ELM_AddMeter() calls (**/Core/Src/*.c)
"""

import argparse
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import yaml

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vars_model import (TYPES, UNIT_SUFFIX, UNIT_TOKENS, Model, VarDef, VarsYamlError,  # noqa: E402
                        legacy_range, load_model)

VARDEFS_DIR = Path(__file__).resolve().parent.parent
DEFAULT_VARS_H = VARDEFS_DIR.parent / "Inc" / "VARS.h"
DEFAULT_MCAN_H = VARDEFS_DIR.parent / "Inc" / "MCAN.h"
RE_NODEID = re.compile(r"#define\s+NODEID_(\w+)\s+(\d+)")

KNOWN_TOP_KEYS = {"version", "nodes", "defaults", "enums", "groups"}
KNOWN_GROUP_KEYS = {"producer", "vars"}
KNOWN_VAR_KEYS = {"id", "name", "legacy", "type", "enum", "unit", "legacy_scale", "producer",
                  "period_ms", "timeout_ms", "on_can", "count", "receivers", "desc"}
RECEIVE_ONLY_NODES = {"RPI"}
MAX_ID = 0xFFFE
U16_MAX = 0xFFFF
NODE_ID_MAX = 15

RE_NAME = re.compile(r"^[A-Z][A-Z0-9_]*$")
RE_C_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
RE_ENUM_ITEM = re.compile(r"^[A-Za-z0-9_]+$")
RE_VARS_H = re.compile(r"#define\s+(VAR_\w+)\s+(\d+)")
RE_ELM_ADD = re.compile(r"ELM_AddMeter\(\s*\w+\s*,\s*\w+\s*,\s*(VAR_\w+)\s*\)")
RE_CONS_WH = re.compile(r"^CONS_(.+)_WH$")
RE_POW_W = re.compile(r"^POW_(.+)_W$")
ELM_GROUP = "el_meters"
ELM_OFFSET = 10


# ---------------------------------------------------------------------------
@dataclass
class Issue:
    severity: str          # 'E' | 'W'
    line: Optional[int]
    where: str
    msg: str

    def format(self, path: str) -> str:
        label = "ERROR  " if self.severity == "E" else "WARNING"
        return f"{label} {path}:{self.line or 0} {self.where}: {self.msg}"

    def sort_key(self):
        return (self.line or 0, self.msg, self.where)


def _is_int(x) -> bool:
    return isinstance(x, int) and not isinstance(x, bool)


def _is_num(x) -> bool:
    return isinstance(x, (int, float)) and not isinstance(x, bool)


# ---------------------------------------------------------------------------
# Raw YAML node walk: precise key lines + duplicate mapping keys
# (the model loader silently keeps the last of duplicated keys)
# ---------------------------------------------------------------------------
def _scan_nodes(path: str):
    with open(path, "r", encoding="utf-8") as f:
        root = yaml.compose(f, Loader=yaml.SafeLoader)
    key_lines: Dict[Tuple, int] = {}
    dups: List[Tuple[Tuple, str, int, int]] = []

    def walk(node, p):
        if isinstance(node, yaml.MappingNode):
            seen = {}
            for k, v in node.value:
                key = str(k.value) if isinstance(k, yaml.ScalarNode) else None
                ln = k.start_mark.line + 1
                if key is not None and key != "<<":
                    if key in seen:
                        dups.append((p, key, ln, seen[key]))
                    else:
                        seen[key] = ln
                    key_lines[p + (key,)] = ln
                walk(v, p + (key,))
        elif isinstance(node, yaml.SequenceNode):
            for i, v in enumerate(node.value):
                key_lines[p + (i,)] = v.start_mark.line + 1
                walk(v, p + (i,))

    if root is not None:
        walk(root, ())
    return key_lines, dups


def parse_vars_h(path) -> Dict[str, int]:
    out = {}
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = RE_VARS_H.search(line.split("//")[0])
            if m:
                out[m.group(1)] = int(m.group(2))
    return out


def scan_elm_calls(src_root) -> List[Tuple[str, int, str]]:
    """[(file relative to src_root, line, VAR_xxx)] of ELM_AddMeter() calls, comments skipped."""
    root = Path(src_root).resolve()
    # do not pick up the validator's own fake test tree unless it is scanned explicitly
    skip_own = VARDEFS_DIR != root and VARDEFS_DIR not in root.parents
    out = []
    for f in sorted(root.glob("**/Core/Src/*.c")):
        if skip_own and VARDEFS_DIR in f.resolve().parents:
            continue
        try:
            text = f.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        # blank out block comments, keep newlines for line numbers
        text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
        for n, line in enumerate(text.splitlines(), 1):
            code = line.split("//")[0]
            for m in RE_ELM_ADD.finditer(code):
                out.append((str(f.relative_to(root)), n, m.group(1)))
    return out


# ---------------------------------------------------------------------------
class _Ctx:
    def __init__(self, model: Model):
        self.m = model
        self.issues: List[Issue] = []
        try:
            self.key_lines, self.dup_keys = _scan_nodes(model.path)
        except (OSError, yaml.YAMLError):
            self.key_lines, self.dup_keys = {}, []
        # (group key, index in vars list) for every VarDef
        self.var_pos: Dict[int, Tuple[str, int]] = {}
        for g in model.groups:
            for i, d in enumerate(g.vars):
                self.var_pos[id(d)] = (g.key, i)

    def add(self, sev, line, where, msg):
        self.issues.append(Issue(sev, line, where, msg))

    def kl(self, *path, default=None):
        return self.key_lines.get(tuple(str(p) if not isinstance(p, int) else p for p in path), default)

    def vline(self, d: VarDef, key: Optional[str] = None) -> Optional[int]:
        g, i = self.var_pos.get(id(d), (d.group, None))
        if key is not None and i is not None:
            ln = self.kl("groups", g, "vars", i, key)
            if ln:
                return ln
        return d.line or self.kl("groups", g, "vars", i) or self.kl("groups", g)

    @staticmethod
    def vwhere(d: VarDef) -> str:
        return f"{d.group}/{d.name if isinstance(d.name, str) else '?'}"

    def ve(self, d, key, msg):
        self.add("E", self.vline(d, key), self.vwhere(d), msg)

    def vw(self, d, key, msg):
        self.add("W", self.vline(d, key), self.vwhere(d), msg)


def _all_defs(m: Model) -> List[VarDef]:
    return [d for g in m.groups for d in g.vars]


def _fmt_ids(ids: List[int]) -> str:
    ids = sorted(ids)
    return str(ids[0]) if len(ids) == 1 else f"{ids[0]}..{ids[-1]} ({len(ids)} ids)"


# ---------------------------------------------------------------------------
# Rule 1: schema
# ---------------------------------------------------------------------------
def _rule_schema(c: _Ctx):
    m = c.m
    raw = m.raw
    for (p, key, ln, first) in c.dup_keys:
        where = "/".join(str(x) for x in p) or "top"
        c.add("E", ln, where, f"duplicate key '{key}' (first at line {first}); YAML keeps only the last one")

    for k in raw:
        if k != "__line__" and k not in KNOWN_TOP_KEYS:
            c.add("W", c.kl(k), "top", f"unknown top-level key '{k}'")
    if "version" not in raw:
        c.add("E", 1, "top", "missing top-level 'version'")
    elif not _is_int(raw["version"]):
        c.add("E", c.kl("version"), "top", f"'version' must be an int, got {raw['version']!r}")
    for k in ("nodes", "groups"):
        if k not in raw:
            c.add("E", 1, "top", f"missing top-level '{k}'")
        elif not isinstance(raw[k], dict):
            c.add("E", c.kl(k), "top", f"'{k}' must be a mapping")
    if "enums" in raw and raw["enums"] is not None and not isinstance(raw["enums"], dict):
        c.add("E", c.kl("enums"), "top", "'enums' must be a mapping")
    if "defaults" in raw and raw["defaults"] is not None and not isinstance(raw["defaults"], dict):
        c.add("E", c.kl("defaults"), "top", "'defaults' must be a mapping")
    for k in m.defaults:
        if k not in KNOWN_VAR_KEYS:
            c.add("W", c.kl("defaults", k), "defaults", f"unknown key '{k}' in defaults")

    # nodes
    seen_ids: Dict[int, str] = {}
    for name, nid in m.nodes.items():
        ln = c.kl("nodes", name) or m.node_lines.get(name)
        if not isinstance(name, str) or not RE_NAME.match(name):
            c.add("E", ln, "nodes", f"invalid node name {name!r} (must match {RE_NAME.pattern})")
        if not _is_int(nid) or not 0 <= nid <= NODE_ID_MAX:
            c.add("E", ln, "nodes", f"node {name}: id must be an int 0..{NODE_ID_MAX}, got {nid!r}")
            continue
        if nid in seen_ids:
            c.add("E", ln, "nodes", f"node {name}: duplicate node id {nid} (also {seen_ids[nid]})")
        else:
            seen_ids[nid] = name

    # groups
    graw = raw.get("groups") if isinstance(raw.get("groups"), dict) else {}
    for g in m.groups:
        gd = graw.get(g.key)
        gln = g.line or c.kl("groups", g.key)
        if not isinstance(gd, dict):
            c.add("E", c.kl("groups", g.key), f"{g.key}", "group must be a mapping with a 'vars' list")
            continue
        if not isinstance(gd.get("vars"), list):
            c.add("E", gln, f"{g.key}", "group has no 'vars' list")
        for k in gd:
            if k != "__line__" and k not in KNOWN_GROUP_KEYS:
                c.add("W", c.kl("groups", g.key, k) or gln, f"{g.key}", f"unknown group key '{k}'")
        if isinstance(gd.get("vars"), list):
            for i, vr in enumerate(gd["vars"]):
                if not isinstance(vr, dict):
                    c.add("E", c.kl("groups", g.key, "vars", i) or gln, f"{g.key}/?",
                          f"var entry #{i + 1} is not a mapping")

    # vars
    for d in _all_defs(c.m):
        if not d.raw and d.line is None:
            continue  # non-mapping entry, reported above
        for k in d.raw:
            if k not in KNOWN_VAR_KEYS:
                c.vw(d, k, f"unknown key '{k}'")
        if "id" not in d.raw:
            c.ve(d, None, "missing 'id'")
        elif not _is_int(d.id):
            c.ve(d, "id", f"'id' must be an int, got {d.id!r}")
        else:
            last = d.id + d.n - 1
            if d.id < 0 or last > MAX_ID:
                c.ve(d, "id", f"id {d.id}{'..' + str(last) if d.is_array else ''} out of range 0..{MAX_ID}")
        if "name" not in d.raw:
            c.ve(d, None, "missing 'name'")
        elif not isinstance(d.name, str) or not RE_NAME.match(d.name):
            c.ve(d, "name", f"invalid name {d.name!r} (must match {RE_NAME.pattern})")
        if not isinstance(d.type, str) or d.type not in TYPES:
            c.ve(d, "type", f"invalid type {d.type!r} (allowed: {', '.join(TYPES)})")
        if d.count is not None and (not _is_int(d.count) or d.count < 2):
            c.ve(d, "count", f"'count' must be an int >= 2, got {d.count!r}")
        if not _is_num(d.legacy_scale) or d.legacy_scale == 0:
            c.ve(d, "legacy_scale", f"'legacy_scale' must be a non-zero number, got {d.legacy_scale!r}")
        if not isinstance(d.on_can, bool):
            c.ve(d, "on_can", f"'on_can' must be true/false, got {d.on_can!r}")
        if d.unit is not None and not isinstance(d.unit, str):
            c.ve(d, "unit", f"'unit' must be a string, got {d.unit!r}")
        rv = d.raw.get("receivers", c.m.defaults.get("receivers"))
        if rv is not None and not isinstance(rv, list):
            c.ve(d, "receivers", "'receivers' must be a list")


# ---------------------------------------------------------------------------
# Rule 2: unique ids (arrays expanded) and unique names (YAML + expanded)
# ---------------------------------------------------------------------------
def _rule_unique(c: _Ctx):
    defs = _all_defs(c.m)

    def report_pairs(index: Dict[Any, List[VarDef]], what: str):
        pairs: Dict[Tuple[int, int], Tuple[VarDef, VarDef, List[Any]]] = {}
        for key, lst in index.items():
            uniq = []
            for d in lst:
                if all(d is not u for u in uniq):
                    uniq.append(d)
            for j in range(1, len(uniq)):
                a, b = uniq[0], uniq[j]
                pk = (id(a), id(b))
                pairs.setdefault(pk, (a, b, []))[2].append(key)
        for a, b, keys in pairs.values():
            if what == "id":
                ks = _fmt_ids(keys)
                c.ve(b, "id", f"duplicate id {ks}: collides with {c.vwhere(a)} (line {c.vline(a, 'id')})")
                c.ve(a, "id", f"duplicate id {ks}: collides with {c.vwhere(b)} (line {c.vline(b, 'id')})")
            else:
                ks = ", ".join(sorted(str(k) for k in keys))
                c.ve(b, "name", f"duplicate name {ks}: also {c.vwhere(a)} (line {c.vline(a, 'name')})")
                c.ve(a, "name", f"duplicate name {ks}: also {c.vwhere(b)} (line {c.vline(b, 'name')})")

    by_id: Dict[int, List[VarDef]] = {}
    by_name: Dict[str, List[VarDef]] = {}
    for d in defs:
        if _is_int(d.id):
            for i in range(d.n):
                by_id.setdefault(d.id + i, []).append(d)
        if isinstance(d.name, str):
            by_name.setdefault(d.name, []).append(d)
            if d.is_array:
                for i in range(d.n):
                    by_name.setdefault(f"{d.name}_{i + 1}", []).append(d)
    report_pairs(by_id, "id")
    report_pairs(by_name, "name")


# ---------------------------------------------------------------------------
# Rule 3: producer / receivers reference nodes
# ---------------------------------------------------------------------------
def _rule_nodes(c: _Ctx):
    nodes = c.m.nodes
    for g in c.m.groups:
        if g.producer is not None:
            ln = c.kl("groups", g.key, "producer") or g.line
            if g.producer not in nodes:
                c.add("E", ln, g.key, f"group producer '{g.producer}' is not a node")
            elif g.producer in RECEIVE_ONLY_NODES:
                c.add("E", ln, g.key, f"group producer '{g.producer}' is a receive-only node")
    for d in _all_defs(c.m):
        if "producer" in d.raw or ("producer" in c.m.defaults and c.m.group(d.group).producer is None):
            if d.producer is not None and d.producer not in nodes:
                c.ve(d, "producer", f"producer '{d.producer}' is not a node")
            elif d.producer in RECEIVE_ONLY_NODES:
                c.ve(d, "producer", f"producer '{d.producer}' is a receive-only node")
        for r in d.receivers:
            if r not in nodes:
                c.ve(d, "receivers", f"receiver '{r}' is not a node")


# ---------------------------------------------------------------------------
# Rule 4: enums
# ---------------------------------------------------------------------------
def _rule_enums(c: _Ctx):
    all_items: Dict[str, Tuple[str, Any]] = {}
    for ename, emap in c.m.enums.items():
        ln = c.kl("enums", ename) or c.m.enum_lines.get(ename)
        where = f"enums/{ename}"
        if not isinstance(ename, str) or not RE_C_IDENT.match(ename):
            c.add("E", ln, where, f"invalid enum name {ename!r} (must be a C identifier)")
        if not isinstance(emap, dict) or not emap:
            c.add("E", ln, where, "enum must be a non-empty mapping {int: Name}")
            continue
        seen: Dict[str, Any] = {}
        for k, v in emap.items():
            kln = c.kl("enums", ename, k) or ln
            if not _is_int(k):
                c.add("E", kln, where, f"enum key {k!r} must be an int")
            if isinstance(v, bool) or v is None:
                c.add("E", kln, where, f"enum item name {v!r} for key {k!r} is a YAML null/boolean "
                                       f"(On/Off/Yes/No/True/False/~) - quote it")
                continue
            if not isinstance(v, str) or not RE_ENUM_ITEM.match(v):
                c.add("E", kln, where, f"enum item name {v!r} invalid (must match {RE_ENUM_ITEM.pattern})")
                continue
            if v in seen:
                c.add("E", kln, where, f"duplicate enum item name '{v}' (keys {seen[v]!r} and {k!r})")
            else:
                seen[v] = k
            cname = f"{ename}_{v}"
            if cname in all_items and all_items[cname][0] != ename:
                c.add("E", kln, where, f"generated enum constant {cname} also produced by enum {all_items[cname][0]}")
            all_items.setdefault(cname, (ename, k))

    for d in _all_defs(c.m):
        if d.type == "enum":
            if d.enum is None:
                c.ve(d, "type", "type enum requires 'enum: <EnumName>'")
            elif not isinstance(d.enum, str) or d.enum not in c.m.enums:
                c.ve(d, "enum", f"enum '{d.enum}' is not defined in 'enums'")
        elif d.enum is not None:
            c.vw(d, "enum", f"'enum' given on non-enum type {d.type!r} (ignored)")


# ---------------------------------------------------------------------------
# Rule 5: legacy {a..b} ranges vs arrays
# ---------------------------------------------------------------------------
def _rule_legacy_range(c: _Ctx):
    for d in _all_defs(c.m):
        if d.legacy is None:
            continue
        if not isinstance(d.legacy, str):
            c.ve(d, "legacy", f"'legacy' must be a string, got {d.legacy!r}")
            continue
        rng = legacy_range(d.legacy)
        if rng:
            a, b = rng
            if not d.is_array:
                c.ve(d, "legacy", f"legacy range {{{a}..{b}}} on a non-array variable")
            elif b - a + 1 != d.count:
                c.ve(d, "legacy", f"legacy range {{{a}..{b}}} has {b - a + 1} items but count is {d.count}")
        elif d.is_array:
            c.ve(d, "legacy", f"array variable needs a '{{a..b}}' range in legacy, got '{d.legacy}'")


# ---------------------------------------------------------------------------
# Rule 6: legacy defines exist in VARS.h with the same id
# ---------------------------------------------------------------------------
def _rule_legacy_vars_h(c: _Ctx, vars_h):
    if vars_h is False:
        return
    path = Path(vars_h) if vars_h else DEFAULT_VARS_H
    try:
        defs = parse_vars_h(path)
    except OSError:
        c.add("E", 0, "VARS.h", f"cannot read {path}; legacy ids cannot be checked")
        return
    for d in _all_defs(c.m):
        if "legacy" not in d.raw and "legacy" not in c.m.defaults:
            c.vw(d, None, "no 'legacy' define (ok for a new variable)")
    for v in c.m.values:
        lg = v.legacy
        if not isinstance(lg, str):
            continue
        if lg not in defs:
            c.ve(v.base, "legacy", f"legacy {lg} not found in VARS.h")
        elif defs[lg] != v.id:
            c.ve(v.base, "legacy", f"legacy {lg} is {defs[lg]} in VARS.h but id is {v.id}"
                 + (f" ({v.name})" if v.index is not None else ""))


# Rule 6b: node ids match NODEID_* in MCAN.h (generated code uses NODEID_<NODE>)
# ---------------------------------------------------------------------------
def _rule_mcan_nodes(c: _Ctx, mcan_h):
    if mcan_h is False:
        return
    path = Path(mcan_h) if mcan_h else DEFAULT_MCAN_H
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            defs = {m.group(1): int(m.group(2))
                    for m in (RE_NODEID.search(line.split("//")[0]) for line in f) if m}
    except OSError:
        c.add("E", 0, "MCAN.h", f"cannot read {path}; node ids cannot be checked")
        return
    producers = {v.producer for v in c.m.values}
    for name, nid in c.m.nodes.items():
        ln = c.m.node_lines.get(name) or 0
        if name not in defs:
            if name in producers:
                c.add("E", ln, f"nodes/{name}", f"producer node has no NODEID_{name} in MCAN.h")
        elif defs[name] != nid:
            c.add("E", ln, f"nodes/{name}", f"node id {nid} but NODEID_{name} is {defs[name]} in MCAN.h")


# ---------------------------------------------------------------------------
# Rule 7: producer, period_ms, timeout_ms
# ---------------------------------------------------------------------------
def _rule_timing(c: _Ctx):
    for d in _all_defs(c.m):
        if not d.raw:
            continue
        if d.producer is None:
            c.ve(d, None, "no producer (set 'producer' on the var or the group)")
        p = d.period_ms
        if p is None:
            c.ve(d, None, "missing 'period_ms'")
        elif not _is_int(p) or p <= 0:
            c.ve(d, "period_ms", f"'period_ms' must be an int > 0, got {p!r}")
        elif p > U16_MAX:
            c.ve(d, "period_ms", f"'period_ms' {p} does not fit uint16")
        t = d.timeout_raw
        if t is None:
            c.ve(d, None, "missing 'timeout_ms' (set it on the var or 'defaults: {timeout_ms: auto}')")
        elif t != "auto" and (not _is_int(t) or t <= 0):
            c.ve(d, "timeout_ms", f"'timeout_ms' must be an int > 0 or 'auto', got {t!r}")
        elif _is_int(d.timeout_ms) and d.timeout_ms > U16_MAX:
            c.ve(d, "timeout_ms", f"timeout_ms {d.timeout_ms}{' (auto = 3 x period)' if t == 'auto' else ''} "
                                  f"does not fit uint16")
        elif _is_int(d.timeout_ms) and _is_int(p) and d.timeout_ms < p:
            c.ve(d, "timeout_ms", f"timeout_ms {d.timeout_ms} is shorter than period_ms {p}")


# ---------------------------------------------------------------------------
# Rule 8: generated C names
# ---------------------------------------------------------------------------
def _rule_c_names(c: _Ctx):
    seen_struct: Dict[str, str] = {}
    for g in c.m.groups:
        ln = g.line or c.kl("groups", g.key)
        if not RE_C_IDENT.match(g.c_member or ""):
            c.add("E", ln, g.key, f"group key gives invalid C member name '{g.c_member}'")
        if g.c_struct in seen_struct:
            c.add("E", ln, g.key, f"struct name {g.c_struct} also generated by group '{seen_struct[g.c_struct]}'")
        else:
            seen_struct[g.c_struct] = g.key
        fields: Dict[str, VarDef] = {}
        for d in g.vars:
            if not isinstance(d.name, str):
                continue
            if not RE_C_IDENT.match(d.c_field or ""):
                c.ve(d, "name", f"generated C field name '{d.c_field}' is not a valid C identifier")
            other = fields.get(d.c_field)
            if other is not None and other.name != d.name:
                c.ve(d, "name", f"C field {g.c_struct}.{d.c_field} also generated by {other.name} "
                                f"(line {c.vline(other, 'name')})")
            fields.setdefault(d.c_field, d)
    seen_vid: Dict[str, Any] = {}
    for v in c.m.values:
        o = seen_vid.get(v.vid)
        if o is not None and o.base is not v.base and o.base.name != v.base.name:
            c.ve(v.base, "name", f"{v.vid} also generated by {o.base.group}/{o.base.name}")
        seen_vid.setdefault(v.vid, v)
    seen_enum: Dict[str, str] = {}
    for ename in c.m.enums:
        t = f"e{ename}"
        if t in seen_enum:
            c.add("E", c.kl("enums", ename), f"enums/{ename}", f"enum type {t} generated twice")
        seen_enum[t] = ename


# ---------------------------------------------------------------------------
# Rule 9: ELM energy/power pairs
# ---------------------------------------------------------------------------
def _rule_elm(c: _Ctx, src_root):
    by_id = {}
    for v in c.m.values:
        by_id.setdefault(v.id, v)
    for v in c.m.values:
        if v.group != ELM_GROUP:
            continue
        mc = RE_CONS_WH.match(v.name)
        if not mc:
            continue
        p = by_id.get(v.id + ELM_OFFSET)
        if p is None:
            c.ve(v.base, "id", f"ELM rule: no variable at id {v.id + ELM_OFFSET} (= id + {ELM_OFFSET}) "
                               f"for power; expected POW_{mc.group(1)}_W")
            continue
        mp = RE_POW_W.match(p.name)
        if not mp:
            c.ve(v.base, "id", f"ELM rule: id {p.id} (= id + {ELM_OFFSET}) is {p.name}, expected POW_*_W")
        elif mp.group(1) != mc.group(1):
            c.vw(v.base, "id", f"ELM rule: id {p.id} is {p.name}, expected POW_{mc.group(1)}_W")
        if mp and p.producer != v.producer:   # ELM on the energy producer writes the power too
            c.ve(p.base, "producer", f"ELM rule: {p.name} producer {p.producer} differs from "
                                     f"{v.name} producer {v.producer}")

    if not src_root:
        return
    by_legacy = {}
    for v in c.m.values:
        if isinstance(v.legacy, str):
            by_legacy.setdefault(v.legacy, v)
    for fname, ln, var in scan_elm_calls(src_root):
        v = by_legacy.get(var)
        src = f"{fname}:{ln}"
        if v is None:
            c.add("W", 0, "src", f"{src}: ELM_AddMeter(..., {var}) - {var} is not a legacy of any YAML variable")
        elif v.group != ELM_GROUP or not RE_CONS_WH.match(v.name):
            c.vw(v.base, "legacy", f"{src}: ELM_AddMeter(..., {var}) but {v.name} is not a CONS_*_WH "
                                   f"variable in group {ELM_GROUP} (ELM writes power to id + {ELM_OFFSET})")


# ---------------------------------------------------------------------------
# Rule 10: compatibility with a baseline (previous released) YAML
# ---------------------------------------------------------------------------
def _rule_baseline(c: _Ctx, baseline):
    base = baseline if isinstance(baseline, Model) else load_model(str(baseline))
    bname = os.path.basename(base.path)
    cur_id: Dict[int, Any] = {}
    cur_name: Dict[str, Any] = {}
    for v in c.m.values:
        cur_id.setdefault(v.id, v)
        cur_name.setdefault(v.name, v)
    for b in base.values:
        v = cur_id.get(b.id)
        if v is None:
            if b.name in cur_name:
                pass  # reported below as id change
            else:
                c.add("W", 0, "baseline", f"id {b.id} ({b.name}) removed since {bname}; never reuse it")
        else:
            if v.name != b.name:
                c.ve(v.base, "id", f"id {b.id} was {b.name} in {bname}, now {v.name} "
                                   f"(ids must not be reused/renamed)")
            if v.type != b.type:
                c.ve(v.base, "type", f"type of id {b.id} changed {b.type!r} -> {v.type!r} (vs {bname})")
            elif v.type == "enum" and v.enum != b.enum:
                c.ve(v.base, "enum", f"enum of id {b.id} changed {b.enum!r} -> {v.enum!r} (vs {bname})")
        n = cur_name.get(b.name)
        if n is not None and n.id != b.id:
            c.ve(n.base, "id", f"{b.name} had id {b.id} in {bname}, now {n.id} (ids must not change)")


# ---------------------------------------------------------------------------
# Rule 11: unit vs name, scale vs integer types
# ---------------------------------------------------------------------------
def _rule_units(c: _Ctx):
    for d in _all_defs(c.m):
        if not isinstance(d.name, str):
            continue
        toks = d.name.split("_")
        tok = toks[-1]
        if len(toks) >= 2 and tok in UNIT_TOKENS:
            mapped = UNIT_SUFFIX.get(d.unit) if isinstance(d.unit, str) else None
            if mapped is None or mapped[0] != tok:
                exp = [u for u, (t, _) in UNIT_SUFFIX.items() if t == tok]
                c.vw(d, "unit" if "unit" in d.raw else "name",
                     f"name ends with unit token _{tok} but unit is {d.unit!r} (expected {' or '.join(map(repr, exp))})")
        if d.type in ("int32", "uint32", "bool", "enum") and _is_num(d.legacy_scale) \
                and float(d.legacy_scale) != int(d.legacy_scale):
            c.vw(d, "legacy_scale", f"non-integer legacy_scale {d.legacy_scale} on {d.type} variable")


# ---------------------------------------------------------------------------
def validate(model: Model, vars_h=None, baseline=None, src_root=None, mcan_h=None) -> List[Issue]:
    c = _Ctx(model)
    _rule_schema(c)
    _rule_unique(c)
    _rule_nodes(c)
    _rule_enums(c)
    _rule_legacy_range(c)
    _rule_legacy_vars_h(c, vars_h)
    _rule_mcan_nodes(c, mcan_h)
    _rule_timing(c)
    _rule_c_names(c)
    _rule_elm(c, src_root)
    if baseline is not None:
        _rule_baseline(c, baseline)
    _rule_units(c)
    # dedupe identical issues, deterministic order
    uniq = {(i.severity, i.line, i.where, i.msg): i for i in c.issues}
    return sorted(uniq.values(), key=Issue.sort_key)


def main(argv=None) -> int:
    try:
        sys.stdout.reconfigure(errors="replace")
        sys.stderr.reconfigure(errors="replace")
    except AttributeError:
        pass
    ap = argparse.ArgumentParser(description="Validate uha_vars.yaml (VARS2)")
    ap.add_argument("yaml")
    ap.add_argument("--vars-h", help=f"legacy VARS.h (default {DEFAULT_VARS_H})")
    ap.add_argument("--mcan-h", help=f"MCAN.h with NODEID_* (default {DEFAULT_MCAN_H})")
    ap.add_argument("--baseline", help="previous released YAML for the compatibility check")
    ap.add_argument("--src-root", help="scan <dir>/**/Core/Src/*.c for ELM_AddMeter() calls")
    ap.add_argument("-q", "--quiet", action="store_true", help="do not print warnings")
    a = ap.parse_args(argv)

    for p, what in ((a.yaml, "file"), (a.vars_h, "file"), (a.mcan_h, "file"), (a.baseline, "file"),
                    (a.src_root, "dir")):
        if p and not (os.path.isdir(p) if what == "dir" else os.path.isfile(p)):
            print(f"FATAL   {p}: {what} not found", file=sys.stderr)
            return 2
    try:
        model = load_model(a.yaml)
        baseline = load_model(a.baseline) if a.baseline else None
    except VarsYamlError as e:
        print(f"FATAL   {e}", file=sys.stderr)
        return 2

    issues = validate(model, vars_h=a.vars_h, baseline=baseline, src_root=a.src_root, mcan_h=a.mcan_h)
    ne = sum(1 for i in issues if i.severity == "E")
    nw = len(issues) - ne
    for i in issues:
        if i.severity == "E" or not a.quiet:
            print(i.format(a.yaml))
    print(f"{ne} errors, {nw} warnings ({len(model.values)} values, max id {model.max_id})")
    return 1 if ne else 0


if __name__ == "__main__":
    sys.exit(main())
