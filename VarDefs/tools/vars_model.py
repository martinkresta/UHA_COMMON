"""
vars_model.py - shared loader for uha_vars.yaml (VARS2)

Loads the YAML, applies `defaults`, `producer` inheritance (container ->
subgroup), `timeout_ms: auto` (= 3 x period_ms), expands `count` arrays and
derives the C names used by the generator.

Layout: groups -> (vars | subgroups -> vars). A group with `vars` is a "leaf"
(one producer, one validity unless `validity: per_field`); a group with
`subgroups` is a container. C access: Vars2.<Group>.<Field> or
Vars2.<Group>.<Subgroup>.<Field>.

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
from typing import Any, Dict, List, Optional, Tuple

import yaml

LINE_KEY = "__line__"

# unit string (YAML `unit`) -> last token of a YAML `name` that denotes this unit
UNIT_NAME_TOKEN = {
    "W": "W",
    "Wh": "WH",
    "V": "V",
    "A": "A",
    "Ah": "AH",
    "°C": "C",
    "l": "L",
    "l/min": "LPM",
    "Pa": "PA",
    "ppm": "PPM",
}

# name tokens that look like a unit
UNIT_TOKENS = set(UNIT_NAME_TOKEN.values())

# unit string -> required C field suffix (`field: Voltage_V`)
FIELD_SUFFIX = {
    "%": "pct",
    "W": "W",
    "Wh": "Wh",
    "V": "V",
    "A": "A",
    "Ah": "Ah",
    "°C": "C",
    "l": "l",
    "l/min": "lpm",
    "Pa": "Pa",
    "ppm": "ppm",
    "m/s": "mps",
}

TYPES = ("float", "int32", "uint32", "bool", "enum")
SEND_MODES = ("periodic", "on_change")   # `send`: periodic (default) | on_change (VMF_ON_CHANGE)
VALIDITY = ("group", "per_field")

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
    group: str                   # path of the leaf group ('GlobalEnergy.Battery')
    grp: Any                     # the leaf Group
    raw: Dict[str, Any]          # YAML entry as written (without line marker)
    line: Optional[int]
    id: Any
    name: Any
    field: Any                   # C field name ('Soc_pct'), as written
    legacy: Any
    type: Any
    enum: Any
    unit: Any
    legacy_scale: Any
    producer: Any                # producer of the leaf group (resolved)
    period_ms: Any
    timeout_ms: Any              # resolved: int, or None if not derivable
    timeout_raw: Any             # as written / defaulted ('auto' or int)
    on_can: Any
    count: Any                   # as written (None if not an array)
    receivers: List[Any]
    desc: Any
    send: Any = "periodic"       # resolved: var, else subgroup, else container, else defaults, else periodic

    @property
    def is_array(self) -> bool:
        return isinstance(self.count, int) and not isinstance(self.count, bool) and self.count > 1

    @property
    def n(self) -> int:
        return self.count if self.is_array else 1

    @property
    def c_field(self) -> str:
        return self.field if isinstance(self.field, str) else ""


@dataclass
class Value:
    """One CAN value (arrays expanded)."""
    id: int
    name: str                    # YAML name; for arrays NAME_<n> (n = 1..count)
    base: VarDef
    index: Optional[int]         # 0-based array index, None for scalars
    legacy: Any                  # expanded legacy define ('VAR_BMS1_CELL3_MV')
    vid: str                     # 'VID_BMS1_CELL_V_3'
    c_ref: str                   # 'PackWorkshop251Ah.CellVoltage_V[2]'  (relative to Vars2)

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
    def send(self): return self.base.send
    @property
    def line(self): return self.base.line


@dataclass
class Group:
    key: str                     # 'Battery'
    path: str                    # 'GlobalEnergy.Battery'
    ypath: Tuple                 # YAML key path ('groups', 'GlobalEnergy', 'subgroups', 'Battery')
    line: Optional[int]
    raw: Dict[str, Any]          # YAML mapping as written (without line marker), {} if not a mapping
    parent: Optional["Group"]
    producer_raw: Any            # as written on this group
    producer: Any                # resolved: own, else container's, else defaults
    struct: Any                  # as written (top-level groups only)
    validity: Any                # resolved: own, else 'group'
    send: Any                    # resolved: own, else container's, else defaults, else 'periodic'
    c_struct: str                # 'sVarsGlobalEnergyBattery', 'sVarsPack', 'sVarsLocalEnergyStorage'
    vars: List[VarDef] = field(default_factory=list)
    subgroups: List["Group"] = field(default_factory=list)

    @property
    def c_member(self) -> str:
        return self.key

    @property
    def is_container(self) -> bool:
        return bool(self.subgroups) or "subgroups" in self.raw

    def leaves(self) -> List["Group"]:
        if self.is_container:
            return [s for s in self.subgroups]
        return [self]


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
    groups: List[Group]          # top-level groups (leaves and containers)
    values: List[Value]          # expanded, sorted by id (values without int id are skipped)

    @property
    def leaf_groups(self) -> List[Group]:
        """All groups that hold `vars`, in YAML order."""
        return [lf for g in self.groups for lf in g.leaves()]

    def group(self, path: str) -> Optional[Group]:
        for g in self.groups:
            if g.path == path:
                return g
            for s in g.subgroups:
                if s.path == path:
                    return s
        return None

    def by_id(self) -> Dict[int, Value]:
        return {v.id: v for v in self.values}

    def by_name(self) -> Dict[str, Value]:
        return {v.name: v for v in self.values}

    @property
    def max_id(self) -> int:
        return max((v.id for v in self.values), default=0)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
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


# ---------------------------------------------------------------------------
# Loader
# ---------------------------------------------------------------------------
def _load_vars(grp: Group, gdict: Dict[str, Any], defaults: Dict[str, Any]):
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
        send = r.get("send", grp.send)
        grp.vars.append(VarDef(
            group=grp.path, grp=grp, raw=r, line=vd.get(LINE_KEY),
            id=merged.get("id"), name=merged.get("name"), field=r.get("field"),
            legacy=merged.get("legacy"),
            type=merged.get("type", "float"), enum=merged.get("enum"), unit=merged.get("unit"),
            legacy_scale=merged.get("legacy_scale", 1),
            producer=grp.producer,
            period_ms=period, timeout_ms=tout, timeout_raw=tout_raw,
            on_can=merged.get("on_can", True), count=merged.get("count"),
            receivers=list(recv) if isinstance(recv, list) else ([] if recv is None else [recv]),
            desc=merged.get("desc"), send=send,
        ))


def _make_group(key, gdict, ypath, parent, defaults) -> Group:
    raw = _strip(gdict) if isinstance(gdict, dict) else {}
    own_prod = raw.get("producer")
    if own_prod is not None:
        producer = own_prod
    elif parent is not None and parent.producer_raw is not None:
        producer = parent.producer_raw
    else:
        producer = defaults.get("producer")
    validity = raw.get("validity", "group")
    if "send" in raw:
        send = raw["send"]
    elif parent is not None:
        send = parent.send
    else:
        send = defaults.get("send", "periodic")
    if parent is None:
        c_struct = "sVars" + str(raw.get("struct") or key)
    else:
        c_struct = "sVars" + str(parent.struct or parent.key) + str(key)
    return Group(key=key, path=key if parent is None else f"{parent.key}.{key}", ypath=ypath,
                 line=gdict.get(LINE_KEY) if isinstance(gdict, dict) else None, raw=raw,
                 parent=parent, producer_raw=own_prod, producer=producer,
                 struct=raw.get("struct"), validity=validity, send=send, c_struct=c_struct)


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
        grp = _make_group(gkey, graw, ("groups", gkey), None, defaults)
        subs = grp.raw.get("subgroups")
        if isinstance(subs, dict):
            for skey, sraw in _strip(subs).items():
                sub = _make_group(skey, sraw, ("groups", gkey, "subgroups", skey), grp, defaults)
                _load_vars(sub, sraw if isinstance(sraw, dict) else {}, defaults)
                grp.subgroups.append(sub)
        else:
            _load_vars(grp, graw if isinstance(graw, dict) else {}, defaults)
        groups.append(grp)

    values: List[Value] = []
    for lf in [lf for g in groups for lf in g.leaves()]:
        for d in lf.vars:
            if not _is_int(d.id) or not isinstance(d.name, str):
                continue
            if d.is_array:
                for i in range(d.n):
                    values.append(Value(
                        id=d.id + i, name=f"{d.name}_{i + 1}", base=d, index=i,
                        legacy=expand_legacy(d.legacy, i),
                        vid=f"VID_{d.name}_{i + 1}",
                        c_ref=f"{lf.path}.{d.c_field}[{i}]"))
            else:
                values.append(Value(
                    id=d.id, name=d.name, base=d, index=None, legacy=d.legacy,
                    vid=f"VID_{d.name}", c_ref=f"{lf.path}.{d.c_field}"))
    values.sort(key=lambda v: (v.id, v.name))

    return Model(path=path, raw=data, version=data.get("version"), nodes=nodes,
                 node_lines=node_lines, defaults=defaults, enums=enums, enum_lines=enum_lines,
                 groups=groups, values=values)


if __name__ == "__main__":
    import sys
    m = load_model(sys.argv[1] if len(sys.argv) > 1 else "uha_vars.yaml")
    print(f"version {m.version}: {len(m.groups)} groups, {len(m.leaf_groups)} leaf groups, "
          f"{len(m.values)} values, max id {m.max_id}")
    for g in m.leaf_groups:
        print(f"\n{g.c_struct} {g.path}  (producer {g.producer}, validity {g.validity})")
        for d in g.vars:
            arr = f"[{d.n}]" if d.is_array else ""
            print(f"  {d.id!s:>4} {d.name:<28} -> {d.c_field}{arr}  {d.type} "
                  f"per={d.period_ms} to={d.timeout_ms} can={d.on_can} send={d.send}")
