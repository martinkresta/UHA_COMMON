"""
test_com.py - PC unit test of UHA_COMMON/Src/COM.c (VARS2 direct access: YAML-driven TX).

Compiles COM.c + VARS2.c + uha_vars_gen.c + test_com.c (MCAN and HAL_GetTick mocked,
stub main.h / APP_INFO.h in stub_com/) with gcc and runs it. The comparison with the code
before direct access (frames, values) is in test_compat.py.

Usage: python tests/test_com.py [--cc gcc]     Exit code 0 = OK.
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
           com_c, os.path.join(SRC, "VARS2.c"),
           os.path.join(SRC, "uha_vars_gen.c"), os.path.join(HERE, "test_com.c"),
           "-I", os.path.join(HERE, "stub_com"), "-I", INC, "-o", exe]
    rc, _ = run(cmd)
    return rc == 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", default="gcc")
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
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("test_com PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
