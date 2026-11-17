"""
test_cmd_fwd.py - PC unit test of the ELECON RPi command forwarding (CMD_spec.md sec.3).

Copies ASW_ELECON/Core/{Src/RPISERP.c,Src/SCOM.c,Inc/*.h} to a temp dir (so the
stub main.h in stub_rsp/ is used instead of the CubeMX one), compiles them with
VARS2.c, uha_vars_gen.c, circbuf.c and test_cmd_fwd.c (simulated UART RX) and
runs the result.

Usage: python tests/test_cmd_fwd.py [--cc gcc]     Exit code 0 = OK.
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VARDEFS = os.path.dirname(HERE)
COMMON = os.path.dirname(VARDEFS)
INC = os.path.join(COMMON, "Inc")
SRC = os.path.join(COMMON, "Src")
ELECON = os.path.normpath(os.path.join(COMMON, "..", "ASW_ELECON", "Core"))


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
    tmp = tempfile.mkdtemp(prefix="cmdfwd_ut_")
    try:
        work = os.path.join(tmp, "elecon")
        os.makedirs(work)
        for f in glob.glob(os.path.join(ELECON, "Inc", "*.h")):
            if os.path.basename(f) != "main.h":
                shutil.copy(f, work)
        for f in ("RPISERP.c", "SCOM.c"):
            shutil.copy(os.path.join(ELECON, "Src", f), work)
        exe = os.path.join(tmp, "test_cmd_fwd.exe")
        cmd = [a.cc, "-std=gnu99", "-Wall", "-Wextra", "-Wno-pointer-to-int-cast", "-Wno-unused-parameter",
               os.path.join(SRC, "uha_vars_gen.c"), os.path.join(SRC, "VARS2.c"), os.path.join(SRC, "circbuf.c"),
               os.path.join(HERE, "test_cmd_fwd.c"),
               "-I", os.path.join(HERE, "stub_rsp"), "-I", work, "-I", INC, "-o", exe]
        rc, out = run(cmd)
        if rc != 0:
            print("FAIL: test_cmd_fwd does not compile")
            return 1
        rc, _ = run([exe])
        if rc != 0:
            return 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("test_cmd_fwd PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
