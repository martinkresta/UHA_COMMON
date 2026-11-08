"""
test_readlayer.py - PC unit test of the VARS2 read layer with gcc.

Compiles VARS2.c + uha_vars_gen.c + test_readlayer.c (which mocks HAL_GetTick(),
VAR_GetVariable() and MCAN_SendFrame() - VARS.c/MCAN.c are not linked) with
`gcc -std=c99 -Wall -Wextra -pedantic`, then runs the resulting test program.

Usage (from UHA_COMMON/VarDefs or anywhere):
    python tests/test_readlayer.py [--cc gcc]
Exit code 0 = OK.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VARDEFS = os.path.dirname(HERE)
STUB = os.path.join(HERE, "stub")
INC = os.path.normpath(os.path.join(VARDEFS, "..", "Inc"))
SRC = os.path.normpath(os.path.join(VARDEFS, "..", "Src"))


def run(cmd):
    print("$ " + " ".join(cmd))
    r = subprocess.run(cmd, capture_output=True, text=True)
    out = (r.stdout + r.stderr).strip()
    if out:
        print(out)
    return r.returncode, out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", default="gcc")
    a = ap.parse_args()
    if shutil.which(a.cc) is None:
        print(f"ERROR: compiler '{a.cc}' not found")
        return 2

    flags = ["-std=c99", "-Wall", "-Wextra", "-pedantic", "-Wno-overflow"]  # -Wno-overflow: pre-existing mDummyVar in VARS.c
    incs = ["-I", STUB, "-I", INC]
    srcs = [
        os.path.join(SRC, "uha_vars_gen.c"),
        os.path.join(SRC, "VARS2.c"),
        os.path.join(SRC, "VARS.c"),
        os.path.join(HERE, "test_readlayer.c"),
    ]
    tmp = tempfile.mkdtemp(prefix="vars2_ut_")
    try:
        exe = os.path.join(tmp, "test_readlayer.exe")
        rc, out = run([a.cc] + flags + srcs + incs + ["-o", exe])
        if rc != 0 or out:
            print("FAIL: test_readlayer does not compile cleanly")
            return 1
        print("compile OK (0 warnings)")

        rc, _ = run([exe])
        if rc != 0:
            return 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("test_readlayer PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
