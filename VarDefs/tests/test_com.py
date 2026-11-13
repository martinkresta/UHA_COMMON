"""
test_com.py - PC unit test of UHA_COMMON/Src/COM.c (VARS2 step 6b part A: old 0x110 frame removed).

Compiles COM.c + VARS.c + VARS2.c + uha_vars_gen.c + test_com.c (MCAN and HAL_GetTick mocked,
stub main.h / APP_INFO.h in stub_com/) with gcc and runs it.

With --ref <old COM.c> (e.g. from `git show <commit>:Src/COM.c`) the same test is also built
with the old COM.c and the CAN TX traces are compared: the 0x180+node frames (ticks and
bytes) must be identical, i.e. the stream timing is unchanged; the old build additionally
sent 0x110+node.

Usage: python tests/test_com.py [--cc gcc] [--ref old_COM.c]     Exit code 0 = OK.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
COMMON = os.path.dirname(os.path.dirname(HERE))
INC = os.path.join(COMMON, "Inc")
SRC = os.path.join(COMMON, "Src")


def run(cmd, show=True):
    if show:
        print("$ " + " ".join(cmd))
    r = subprocess.run(cmd, capture_output=True, text=True)
    out = (r.stdout + r.stderr).strip()
    if show and out:
        print(out)
    return r.returncode, r.stdout


def build(cc, com_c, exe):
    cmd = [cc, "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
           com_c, os.path.join(SRC, "VARS.c"), os.path.join(SRC, "VARS2.c"),
           os.path.join(SRC, "uha_vars_gen.c"), os.path.join(HERE, "test_com.c"),
           "-I", os.path.join(HERE, "stub_com"), "-I", INC, "-o", exe]
    rc, _ = run(cmd)
    return rc == 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", default="gcc")
    ap.add_argument("--ref", help="old COM.c to compare the stream timing with")
    a = ap.parse_args()
    if shutil.which(a.cc) is None:
        print(f"ERROR: compiler '{a.cc}' not found")
        return 2
    tmp = tempfile.mkdtemp(prefix="com_ut_")
    try:
        exe = os.path.join(tmp, "test_com.exe")
        if not build(a.cc, os.path.join(SRC, "COM.c"), exe):
            print("FAIL: test_com does not compile")
            return 1
        rc, _ = run([exe])
        if rc != 0:
            return 1
        if a.ref:
            ref_c = os.path.join(tmp, "COM_ref.c")
            shutil.copy(a.ref, ref_c)
            ref_exe = os.path.join(tmp, "test_com_ref.exe")
            if not build(a.cc, ref_c, ref_exe):
                print("FAIL: reference build does not compile")
                return 1
            _, new_trace = run([exe, "--trace"], show=False)
            _, ref_trace = run([ref_exe, "--trace"], show=False)
            new_lines = new_trace.splitlines()
            ref_lines = ref_trace.splitlines()
            new_v2 = [l for l in new_lines if l.startswith("T ") and l.split()[2].startswith("18")]
            ref_v2 = [l for l in ref_lines if l.startswith("T ") and l.split()[2].startswith("18")]
            ref_old = [l for l in ref_lines if l.startswith("T ") and l.split()[2].startswith("11")]
            new_old = [l for l in new_lines if l.startswith("T ") and l.split()[2].startswith("11")]
            new_rx = [l for l in new_lines if l.startswith("R ")]
            ref_rx = [l for l in ref_lines if l.startswith("R ")]
            print(f"ref: {len(ref_v2)} x 0x18x, {len(ref_old)} x 0x11x;  new: {len(new_v2)} x 0x18x, {len(new_old)} x 0x11x")
            print(f"RX ref: {ref_rx}  new: {new_rx}")
            ok = new_v2 == ref_v2 and len(new_old) == 0 and len(ref_old) > 0 and new_rx == ref_rx
            if not ok:
                print("FAIL: traces differ from the reference")
                return 1
            print("reference comparison: 0x18x traces identical, stream timing unchanged")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("test_com PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
