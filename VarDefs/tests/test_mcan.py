"""
test_mcan.py - PC unit test of the MCAN TX supervision (stall detection, recovery, ring-full counting, ABOM).

Compiles UHA_COMMON/Src/MCAN.c + circbuf.c with test_mcan.c and the simulated bxCAN in stub_mcan/main.h,
twice: default config (self-reset after 5) and MCAN_TX_STALL_RESET_AFTER=0 (never reset).

Usage: python tests/test_mcan.py [--cc gcc]     Exit code 0 = OK.
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
    tmp = tempfile.mkdtemp(prefix="mcan_ut_")
    try:
        for defs in (["-DMCAN_UNIT_TEST"], ["-DMCAN_UNIT_TEST", "-DMCAN_TX_STALL_RESET_AFTER=0u"]):
            exe = os.path.join(tmp, "test_mcan%d.exe" % len(defs))
            cmd = [a.cc, "-std=gnu99", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-Wno-unused-variable",
                   *defs, os.path.join(SRC, "MCAN.c"), os.path.join(SRC, "circbuf.c"), os.path.join(HERE, "test_mcan.c"),
                   "-I", os.path.join(HERE, "stub_mcan"), "-I", INC, "-o", exe]
            rc, _ = run(cmd)
            if rc != 0:
                print("FAIL: test_mcan does not compile")
                return 1
            rc, _ = run([exe])
            if rc != 0:
                return 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("test_mcan PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
