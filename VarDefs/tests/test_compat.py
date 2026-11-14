"""
test_compat.py - old vs new: frames on CAN / tunnel and VAR_GetVariable() results identical.

Builds tests/test_compat.c twice with gcc:
  ref  with UHA_COMMON/Inc + Src (VARS*, COM, uha_vars_gen) from a reference commit
       (default e28d6a8 = main before direct access: Vars[], COM stream list, VARS2_SendLegacy)
  new  with the current working tree
and compares the outputs line by line (see test_compat.c). The YAML ids, scales, periods and
producers must be the same in both (direct access changed only the layout).

Usage: python tests/test_compat.py [--cc gcc] [--ref e28d6a8]     Exit code 0 = OK.
"""

import argparse
import io
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
COMMON = os.path.dirname(os.path.dirname(HERE))
STUB = os.path.join(HERE, "stub_com")
SOURCES = ["VARS.c", "VARS2.c", "COM.c", "uha_vars_gen.c"]
NODES = [1, 2, 3, 4, 5, 7]   # IOBOARD_D, IOBOARD_U, TECHM, ELECON, RECU, ELECON_D


def run(cmd, show=True):
    if show:
        print("$ " + " ".join(cmd))
    r = subprocess.run(cmd, capture_output=True, text=True)
    if show and (r.stdout + r.stderr).strip() and r.returncode != 0:
        print((r.stdout + r.stderr).strip()[:4000])
    return r.returncode, r.stdout, r.stderr


def build(cc, root, exe):
    inc, src = os.path.join(root, "Inc"), os.path.join(root, "Src")
    cmd = [cc, "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter"]
    cmd += [os.path.join(src, s) for s in SOURCES] + [os.path.join(HERE, "test_compat.c")]
    cmd += ["-I", STUB, "-I", inc, "-o", exe]
    rc, out, err = run(cmd)
    return rc == 0


def export_ref(ref, dest):
    p = subprocess.run(["git", "-C", COMMON, "archive", "--format=tar", ref, "Inc", "Src"],
                       capture_output=True)
    if p.returncode != 0:
        print(p.stderr.decode(errors="replace"))
        return False
    with tarfile.open(fileobj=io.BytesIO(p.stdout)) as t:
        t.extractall(dest)
    return True


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", default="gcc")
    ap.add_argument("--ref", default="e28d6a8", help="reference commit (default: main before direct access)")
    a = ap.parse_args()
    if shutil.which(a.cc) is None:
        print(f"ERROR: compiler '{a.cc}' not found")
        return 2
    tmp = tempfile.mkdtemp(prefix="compat_ut_")
    try:
        ref_root = os.path.join(tmp, "ref")
        os.makedirs(ref_root)
        if not export_ref(a.ref, ref_root):
            print(f"FAIL: cannot export {a.ref}")
            return 1
        ref_exe = os.path.join(tmp, "compat_ref.exe")
        new_exe = os.path.join(tmp, "compat_new.exe")
        if not build(a.cc, ref_root, ref_exe):
            print("FAIL: reference build")
            return 1
        if not build(a.cc, COMMON, new_exe):
            print("FAIL: new build")
            return 1
        ref_l, new_l = [], []
        for node in NODES:
            rc1, ref_out, _ = run([ref_exe, str(node)], show=False)
            rc2, new_out, _ = run([new_exe, str(node)], show=False)
            if rc1 or rc2:
                print(f"FAIL: test program exit code (node {node})")
                return 1
            ref_l += ref_out.splitlines()
            new_l += new_out.splitlines()
        kinds = {}
        for l in new_l:
            kinds[l[0]] = kinds.get(l[0], 0) + 1
        print("lines per kind (new): " + ", ".join(f"{k}={n}" for k, n in sorted(kinds.items())))
        none = [l for l in new_l if l.endswith(" none")]
        if none:
            print(f"FAIL: {len(none)} own variables never sent, e.g. {none[:3]}")
            return 1
        bad = [l for l in ref_l + new_l if l.startswith("X ") and l != "X bad-cobid 0"]
        if bad:
            print(f"FAIL: frames with a wrong COB-ID: {bad}")
            return 1
        if ref_l != new_l:
            diffs = [(r, n) for r, n in zip(ref_l, new_l) if r != n]
            print(f"FAIL: {len(diffs)} differing lines (ref {len(ref_l)} lines, new {len(new_l)} lines)")
            for r, n in diffs[:25]:
                print(f"  ref: {r}\n  new: {n}")
            return 1
        print(f"{len(new_l)} lines identical (ref {a.ref} vs working tree)")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("test_compat PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
