"""
gen_all.py - one command for VARS2: validate uha_vars.yaml, then generate FW code.

Usage (from anywhere):
    python tools/gen_all.py [yaml] [--baseline OLD.yaml] [--src-root DIR] [-o OUTDIR]

Defaults: yaml = ../uha_vars.yaml (next to this tools/ folder),
          uha_vars_gen.h -> ../../Inc, uha_vars_gen.c -> ../../Src (UHA_COMMON/Inc, Src).
          -o overrides both with a single directory.
Generation is skipped when the validator reports any error (exit code 1).
"""

import argparse
import os
import sys

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
VARDEFS_DIR = os.path.dirname(TOOLS_DIR)
sys.path.insert(0, TOOLS_DIR)

from vars_model import load_model, VarsYamlError  # noqa: E402
from vars_validate import validate  # noqa: E402
from vars_gen_fw import generate, GenError  # noqa: E402


def main() -> int:
    try:
        sys.stdout.reconfigure(errors="replace")
    except AttributeError:
        pass

    ap = argparse.ArgumentParser(description="Validate uha_vars.yaml and generate FW code")
    ap.add_argument("yaml", nargs="?", default=os.path.join(VARDEFS_DIR, "uha_vars.yaml"))
    ap.add_argument("--mcan-h", default=os.path.join(VARDEFS_DIR, "..", "Inc", "MCAN.h"))
    ap.add_argument("--baseline", help="previous released YAML for the compatibility check")
    ap.add_argument("--src-root", help="workspace root to cross-check ELM_AddMeter() calls")
    ap.add_argument("--inc-dir", default=os.path.normpath(os.path.join(VARDEFS_DIR, "..", "Inc")))
    ap.add_argument("--src-dir", default=os.path.normpath(os.path.join(VARDEFS_DIR, "..", "Src")))
    ap.add_argument("-o", "--outdir", help="override: write both files into this single dir")
    args = ap.parse_args()
    inc_dir = args.outdir if args.outdir else args.inc_dir
    src_dir = args.outdir if args.outdir else args.src_dir

    try:
        model = load_model(args.yaml)
        baseline = load_model(args.baseline) if args.baseline else None
    except VarsYamlError as e:
        print(f"ERROR   {e}")
        return 2

    issues = validate(model, baseline=baseline, src_root=args.src_root,
                      mcan_h=args.mcan_h)
    for i in issues:
        print(i.format(args.yaml))
    nerr = sum(1 for i in issues if i.severity == "E")
    nwarn = len(issues) - nerr
    print(f"validate: {nerr} errors, {nwarn} warnings "
          f"({len(model.values)} values, max id {model.max_id})")
    if nerr:
        print("generation skipped (fix the errors first)")
        return 1

    try:
        generate(model, inc_dir, src_dir)
    except GenError as e:
        print(f"ERROR   {e}")
        return 1
    print("done")
    return 0


if __name__ == "__main__":
    sys.exit(main())
