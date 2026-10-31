"""
compile_check.py - PC compile + sanity check of the generated VARS2 FW code.

1. gcc -std=c99 -Wall -Wextra -c generated/uha_vars_gen.c (stub main.h before
   UHA_COMMON/Inc); any warning or error fails.
2. Builds and runs a small test program (temp dir) that checks the tables
   at runtime (ids sorted, index table consistent, pValue inside Vars2, ...).

Usage (from UHA_COMMON/VarDefs or anywhere):
    python tests/compile_check.py [--cc gcc]
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
GEN = os.path.join(VARDEFS, "generated")
STUB = os.path.join(HERE, "stub")
INC = os.path.normpath(os.path.join(VARDEFS, "..", "Inc"))

TEST_PROGRAM = r"""
#include <stdio.h>
#include <stddef.h>
#include "uha_vars_gen.h"
#include "VARS.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
  unsigned i, j, known = 0, oncan = 0;
  const char* lo = (const char*)&Vars2;
  const char* hi = (const char*)(&Vars2 + 1);

  CHECK(sizeof(sVars) == 4u * VARS_COUNT);
  for (i = 0; i < VARS_COUNT; i++)
  {
    const char* p = (const char*)VarMeta[i].pValue;
    if (i > 0) CHECK(VarMeta[i].Id > VarMeta[i - 1].Id);
    CHECK(VarMeta[i].Id <= VAR_MAX_ID);
    CHECK(VarIdToIndex[VarMeta[i].Id] == i);
    CHECK(p >= lo && p + 4 <= hi);
    CHECK(((size_t)(p - lo)) % 4u == 0u);
    CHECK(VarMeta[i].Type <= VT_ENUM);
    CHECK(VarMeta[i].TimeoutMs >= VarMeta[i].PeriodMs);
    CHECK(VarMeta[i].LegacyScale > 0.0f);
    if (VarMeta[i].Flags & VMF_ON_CAN) oncan++;
    if (i > 0) for (j = 0; j < i; j++) if (VarMeta[j].pValue == VarMeta[i].pValue) { CHECK(0 && "duplicate pValue"); }
  }
  for (i = 0; i <= VAR_MAX_ID; i++)
    if (VarIdToIndex[i] != VAR_IDX_UNKNOWN) { known++; CHECK(VarIdToIndex[i] < VARS_COUNT); }
  CHECK(known == VARS_COUNT);
  CHECK(oncan == VARS_ON_CAN_COUNT);
  CHECK(VarMeta[VARS_COUNT - 1].Id == VAR_MAX_ID);

  /* spot values */
  CHECK(VID_BMS1_SOC == 20);
  CHECK(VarMeta[VarIdToIndex[VID_BMS1_SOC]].LegacyId == VAR_BMS1_SOC);
  CHECK(VAR_BMS1_SOC == 20);
  CHECK(VID_BMS1_CELL_V_16 == 195);
  CHECK(VarMeta[VarIdToIndex[VID_BMS1_CELL_V_3]].pValue == (void*)&Vars2.Bms1.Cell_V[2]);
  CHECK(VarMeta[VarIdToIndex[VID_BMS1_CELL_V_3]].LegacyId == VAR_BMS1_CELL3_MV);

  printf("VARS_COUNT=%u VAR_MAX_ID=%u VARS_ON_CAN_COUNT=%u\n",
         (unsigned)VARS_COUNT, (unsigned)VAR_MAX_ID, (unsigned)VARS_ON_CAN_COUNT);
  printf("ARM size estimate: VarMeta %u B (flash), VarIdToIndex %u B (flash), Vars2 %u B (RAM)\n",
         20u * VARS_COUNT, 2u * (VAR_MAX_ID + 1u), 4u * VARS_COUNT);
  printf("PC: sizeof(sVarMeta)=%u, sizeof(sVars)=%u\n", (unsigned)sizeof(sVarMeta), (unsigned)sizeof(sVars));
  printf(fails ? "RUNTIME CHECK FAILED (%d)\n" : "runtime checks OK\n", fails);
  return fails ? 1 : 0;
}
"""


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

    flags = ["-std=c99", "-Wall", "-Wextra", "-pedantic"]
    incs = ["-I", GEN, "-I", STUB, "-I", INC]
    src = os.path.join(GEN, "uha_vars_gen.c")
    tmp = tempfile.mkdtemp(prefix="vars2_cc_")
    try:
        obj = os.path.join(tmp, "uha_vars_gen.o")
        rc, out = run([a.cc] + flags + ["-c", src] + incs + ["-o", obj])
        if rc != 0 or out:
            print("FAIL: uha_vars_gen.c does not compile cleanly")
            return 1
        print("compile OK (0 warnings)")

        test_c = os.path.join(tmp, "test_vars2.c")
        with open(test_c, "w", encoding="ascii", newline="\n") as f:
            f.write(TEST_PROGRAM)
        exe = os.path.join(tmp, "test_vars2.exe")
        rc, out = run([a.cc] + flags + [test_c, obj] + incs + ["-o", exe])
        if rc != 0 or out:
            print("FAIL: test program does not compile cleanly")
            return 1
        rc, _ = run([exe])
        if rc != 0:
            return 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("compile_check PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
