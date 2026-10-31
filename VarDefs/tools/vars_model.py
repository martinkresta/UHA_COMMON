"""
vars_model.py - shared loader for uha_vars.yaml (VARS2)

Loads the YAML, applies `defaults`, group `producer` inheritance,
`timeout_ms: auto` (= 3 x period_ms), expands `count` arrays and derives
the C names used by the generators.

The model does NOT validate. It is lenient on malformed input (missing keys
become None) so that the validator can report problems with YAML locations.
Only unparseable YAML raises VarsYamlError.

Usage:
    from vars_model import load_model
    m = load_model("uha_vars.yaml")
    for v in m.values: print(v.id, v.vid, v.c_ref)
"""

import re
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

import yaml

LINE_KEY = "__line__"

# unit string (YAML `unit`) -> (name token, C suffix spelling)
UNIT_SUFFIX = {
    "W": ("W", "W"),
    "Wh": ("WH", "Wh"),
    "V": ("V", "V"),
    "A": ("A", "A"),
    "Ah": ("AH", "Ah"),
    "°C": ("C", "C"),
    "l": ("L", "L"),
    "l/min": ("LPM", "Lpm"),
    "Pa": ("PA", "Pa"),
    "ppm": ("PPM", "ppm"),
}

# name token -> C suffix spelling (for tokens that look like a unit)
UNIT_TOKENS = {tok: suf for tok, suf in UNIT_SUFFIX.values()}

TYPES = ("float", "int32", "uint32", "bool", "enum")

_LEGACY_RANGE = re.compile(r"\{(\d+)\.\.(\d+)\}")


class VarsYamlError(Exception):
    pass


# ---------------------------------------------------------------------------
# YAML loader that records line numbers of mappings
# ---------------------------------------------------------------------------
class _LineLoader(yaml.SafeLoader):
    pass


def _construct_mapping(loader, node, deep=False):
    loader.flatten_mapping(node)
    mapping = {}
    for key_node, value_node in node.value:
        key = loader.construct_object(key_node, deep=deep)
        value = loader.construct_object(value_node, deep=deep)
        mapping[key] = value
    mapping[LINE_KEY] = node.start_mark.line + 1
    return mapping


_LineLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, _construct_mapping)


def _strip(d):
    """Copy of a mapping without the line marker."""
    if not isinstance(d, dict):
        return d
    return {k: v for k, v in d.items() if k != LINE_KEY}


# ---------------------------------------------------------------------------
# Model classes
# ---------------------------------------------------------------------------
@dataclass
class VarDef:
    """One entry of a group's `vars` list, defaults applied, not expanded."""
    group: str
    raw: Dict[str, Any]          # YAML entry as written (without line marker)
    line: Optional[int]
    id: Any
    name: Any
    legacy: Any
    type: Any
    enum: Any
    unit: Any
    legacy_scale: Any
    producer: Any
    period_ms: Any
    timeout_ms: Any              # resolved: int, or None if not derivable
    timeout_raw: Any             # as written / defaulted ('auto' or int)
    on_can: Any
    count: Any                   # as written (None if not an array)
    receivers: List[Any]
    desc: Any
    c_field: str = ""            # C field name inside the group struct ('Cell_V')

    @property
    def is_array(self) -> bool:
        return isinstance(self.count, int) and not isinstance(self.count, bool) and self.count > 1

    @property
    def n(self) -> int:
        return self.count if self.is_array else 1


@dataclass
class Value:
    """One CAN value (arrays expanded)."""
    id: int
    name: str                    # YAML name; for arrays NAME_<n> (n = 1..count)
    base: VarDef
    index: Optional[int]         # 0-based array index, None for scalars
    legacy: Any                  # expanded legacy define ('VAR_BMS1_CELL3_MV')
    vid: str                     # 'VID_BMS1_CELL_V_3'
    c_ref: str                   # 'Bms1.Cell_V[2]'  (relative to Vars2)

    # convenience pass-through
    @property
    def group(self): return self.base.group
    @property
    def type(self): return self.base.type
    @property
    def enum(self): return self.base.enum
    @property
    def unit(self): return self.base.unit
    @property
    def legacy_scale(self): return self.base.legacy_scale
    @property
    def producer(self): return self.base.producer
    @property
    def period_ms(self): return self.base.period_ms
    @property
    def timeout_ms(self): return self.base.timeout_ms
    @property
    def on_can(self): return self.base.on_can
    @property
    def receivers(self): return self.base.receivers
    @property
    def line(self): return self.base.line


@dataclass
class Group:
    key: str                     # 'global_energy'
    line: Optional[int]
    producer: Any
    c_member: str                # 'GlobalEnergy'
    c_struct: str                # 'sVarsGlobalEnergy'
    prefix: List[str]            # name tokens dropped from C field names
    vars: List[VarDef] = field(default_factory=list)


@dataclass
class Model:
    path: str
    raw: Dict[str, Any]
    version: Any
    nodes: Dict[str, Any]        # name -> node id
    node_lines: Dict[str, int]
    defaults: Dict[str, Any]
    enums: Dict[str, Dict[Any, Any]]   # name -> {value: name}
    enum_lines: Dict[str, int]
    groups: List[Group]
    values: List[Value]          # expanded, sorted by id (values without int id are skipped)

    def group(self, key: str) -> Optional[Group]:
        for g in self.groups:
            if g.key == key:
                return g
        return None

    def by_id(self) -> Dict[int, Value]:
        return {v.id: v for v in self.values}

    def by_name(self) -> Dict[str, Value]:
        return {v.name: v for v in self.values}

    @property
    def max_id(self) -> int:
        return max((v.id for v in self.values), default=0)


# ---------------------------------------------------------------------------
# Naming helpers
# ---------------------------------------------------------------------------
def camel(tokens: List[str]) -> str:
    """['CELL', 'TEMP'] -> 'CellTemp'"""
    return "".join(t[:1].upper() + t[1:].lower() for t in tokens if t)


def group_c_member(key: str) -> str:
    """'global_energy' -> 'GlobalEnergy'"""
    return camel(str(key).upper().split("_"))


def unit_suffix(name: str, unit: Any) -> Optional[str]:
    """C unit suffix if the last name token matches the `unit` field, else None."""
    tokens = str(name).split("_")
    if len(tokens) < 2 or unit not in UNIT_SUFFIX:
        return None
    tok, suf = UNIT_SUFFIX[unit]
    return suf if tokens[-1] == tok else None


def _body_tokens(name: str, unit: Any) -> List[str]:
    tokens = str(name).split("_")
    if unit_suffix(name, unit) is not None:
        tokens = tokens[:-1]
    return tokens


def common_prefix(defs: List[VarDef]) -> List[str]:
    """Longest leading token run shared by all names of a group, such that every
    name keeps at least one non-unit token. Empty for groups with < 2 names."""
    names = [d for d in defs if isinstance(d.name, str) and d.name]
    if len(names) < 2:
        return []
    bodies = [_body_tokens(d.name, d.unit) for d in names]
    k = 0
    while all(len(b) > k + 1 for b in bodies) and len({b[k] for b in bodies}) == 1:
        k += 1
    return bodies[0][:k]


def c_field_name(name: str, unit: Any, prefix: List[str]) -> str:
    """LEMON style: CamelCase + _Unit, group prefix dropped.
    'BMS1_CELL_TEMP_C' (unit °C, prefix ['BMS1']) -> 'CellTemp_C'"""
    body = _body_tokens(name, unit)[len(prefix):]
    suf = unit_suffix(name, unit)
    return camel(body) + ("_" + suf if suf else "")


def expand_legacy(legacy: Any, index: int) -> Any:
    """'VAR_BMS1_CELL{1..16}_MV', 2 -> 'VAR_BMS1_CELL3_MV' (range start + index)."""
    if not isinstance(legacy, str):
        return legacy
    m = _LEGACY_RANGE.search(legacy)
    if not m:
        return legacy
    return legacy[:m.start()] + str(int(m.group(1)) + index) + legacy[m.end():]


def legacy_range(legacy: Any):
    """(start, end) of a '{a..b}' pattern in a legacy name, or None."""
    if not isinstance(legacy, str):
        return None
    m = _LEGACY_RANGE.search(legacy)
    return (int(m.group(1)), int(m.group(2))) if m else None


# ---------------------------------------------------------------------------
# Loader
# ---------------------------------------------------------------------------
def _is_int(x) -> bool:
    return isinstance(x, int) and not isinstance(x, bool)


def load_yaml(path: str) -> Dict[str, Any]:
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.load(f, Loader=_LineLoader)
    except (OSError, yaml.YAMLError) as e:
        raise VarsYamlError(f"{path}: {e}") from e
    if not isinstance(data, dict):
        raise VarsYamlError(f"{path}: top level is not a mapping")
    return data


def load_model(path: str) -> Model:
    data = load_yaml(path)

    nodes_raw = data.get("nodes") if isinstance(data.get("nodes"), dict) else {}
    nodes = _strip(nodes_raw)
    node_lines = {k: nodes_raw.get(LINE_KEY) for k in nodes}

    defaults = _strip(data.get("defaults")) if isinstance(data.get("defaults"), dict) else {}

    enums_raw = data.get("enums") if isinstance(data.get("enums"), dict) else {}
    enums = {}
    enum_lines = {}
    for ename, emap in _strip(enums_raw).items():
        enum_lines[ename] = emap.get(LINE_KEY) if isinstance(emap, dict) else enums_raw.get(LINE_KEY)
        enums[ename] = _strip(emap) if isinstance(emap, dict) else emap

    groups: List[Group] = []
    groups_raw = data.get("groups") if isinstance(data.get("groups"), dict) else {}
    for gkey, graw in _strip(groups_raw).items():
        gdict = graw if isinstance(graw, dict) else {}
        member = group_c_member(gkey)
        grp = Group(key=gkey, line=gdict.get(LINE_KEY), producer=gdict.get("producer"),
                    c_member=member, c_struct="sVars" + member, prefix=[])
        vlist = gdict.get("vars") if isinstance(gdict.get("vars"), list) else []
        for vraw in vlist:
            vd = vraw if isinstance(vraw, dict) else {}
            r = _strip(vd)
            merged = dict(defaults)
            merged.update(r)
            period = merged.get("period_ms")
            tout_raw = merged.get("timeout_ms")
            if tout_raw == "auto":
                tout = 3 * period if _is_int(period) else None
            else:
                tout = tout_raw if _is_int(tout_raw) else None
            recv = merged.get("receivers")
            grp.vars.append(VarDef(
                group=gkey, raw=r, line=vd.get(LINE_KEY),
                id=merged.get("id"), name=merged.get("name"), legacy=merged.get("legacy"),
                type=merged.get("type", "float"), enum=merged.get("enum"), unit=merged.get("unit"),
                legacy_scale=merged.get("legacy_scale", 1),
                producer=merged.get("producer", grp.producer),
                period_ms=period, timeout_ms=tout, timeout_raw=tout_raw,
                on_can=merged.get("on_can", True), count=merged.get("count"),
                receivers=list(recv) if isinstance(recv, list) else ([] if recv is None else [recv]),
                desc=merged.get("desc"),
            ))
        grp.prefix = common_prefix(grp.vars)
        for d in grp.vars:
            if isinstance(d.name, str):
                d.c_field = c_field_name(d.name, d.unit, grp.prefix)
        groups.append(grp)

    values: List[Value] = []
    for grp in groups:
        for d in grp.vars:
            if not _is_int(d.id) or not isinstance(d.name, str):
                continue
            if d.is_array:
                for i in range(d.n):
                    values.append(Value(
                        id=d.id + i, name=f"{d.name}_{i + 1}", base=d, index=i,
                        legacy=expand_legacy(d.legacy, i),
                        vid=f"VID_{d.name}_{i + 1}",
                        c_ref=f"{grp.c_member}.{d.c_field}[{i}]"))
            else:
                values.append(Value(
                    id=d.id, name=d.name, base=d, index=None, legacy=d.legacy,
                    vid=f"VID_{d.name}", c_ref=f"{grp.c_member}.{d.c_field}"))
    values.sort(key=lambda v: (v.id, v.name))

    return Model(path=path, raw=data, version=data.get("version"), nodes=nodes,
                 node_lines=node_lines, defaults=defaults, enums=enums, enum_lines=enum_lines,
                 groups=groups, values=values)


if __name__ == "__main__":
    import sys
    m = load_model(sys.argv[1] if len(sys.argv) > 1 else "uha_vars.yaml")
    print(f"version {m.version}: {len(m.groups)} groups, {len(m.values)} values, max id {m.max_id}")
    for g in m.groups:
        print(f"\n{g.c_struct} {g.c_member}  (prefix {'_'.join(g.prefix) or '-'})")
        for d in g.vars:
            arr = f"[{d.n}]" if d.is_array else ""
            print(f"  {d.id!s:>4} {d.name:<28} -> {d.c_field}{arr}  {d.type} prod={d.producer} "
                  f"per={d.period_ms} to={d.timeout_ms} can={d.on_can}")
