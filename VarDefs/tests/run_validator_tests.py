"""
run_validator_tests.py - runs tools/vars_validate.py on every tests/fixtures/{pass,fail}_*.yaml

Fixture header comments (leading '#' lines):
    # expect: <substr>            an ERROR line must contain <substr> (fail_* fixtures)
    # expect-warning: <substr>    a WARNING line must contain <substr>
    # expect-absent: <substr>     no output line may contain <substr>
    # expect-exit: <n>            expected exit code (default: pass_* -> 0, fail_* -> 1)
    # args: <cli args>            extra CLI args, paths relative to tests/fixtures
                                  (default: --vars-h VARS_test.h)
pass_* must give 0 errors, fail_* >= 1 error. base_*.yaml are baseline inputs, not tests.

Usage:  python tests/run_validator_tests.py [-v]
"""

import shlex
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
FIX = HERE / "fixtures"
VALIDATOR = HERE.parent / "tools" / "vars_validate.py"


def header(path: Path):
    h = {"expect": [], "expect-warning": [], "expect-absent": [], "expect-exit": None, "args": None}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("#"):
            break
        body = line[1:].strip()
        key, sep, val = body.partition(":")
        key = key.strip()
        if not sep or key not in h:
            continue
        val = val.strip()
        if key == "expect-exit":
            h[key] = int(val)
        elif key == "args":
            h[key] = shlex.split(val)
        else:
            h[key].append(val)
    return h


def run_one(path: Path, verbose: bool) -> bool:
    h = header(path)
    is_pass = path.name.startswith("pass_")
    exp_exit = h["expect-exit"] if h["expect-exit"] is not None else (0 if is_pass else 1)
    args = h["args"] if h["args"] is not None else ["--vars-h", "VARS_test.h"]
    p = subprocess.run([sys.executable, str(VALIDATOR), path.name] + args, cwd=FIX,
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = (p.stdout + p.stderr).splitlines()
    errors = [l for l in out if l.startswith("ERROR")]
    warns = [l for l in out if l.startswith("WARNING")]
    problems = []
    if p.returncode != exp_exit:
        problems.append(f"exit {p.returncode}, expected {exp_exit}")
    if is_pass and errors:
        problems.append(f"{len(errors)} unexpected error(s)")
    if not is_pass and exp_exit == 1 and not errors:
        problems.append("no error reported")
    for s in h["expect"]:
        if not any(s in l for l in errors):
            problems.append(f"missing ERROR containing {s!r}")
    for s in h["expect-warning"]:
        if not any(s in l for l in warns):
            problems.append(f"missing WARNING containing {s!r}")
    for s in h["expect-absent"]:
        if any(s in l for l in out):
            problems.append(f"output unexpectedly contains {s!r}")
    ok = not problems
    print(f"{'PASS' if ok else 'FAIL'}  {path.name}" + ("" if ok else "  -> " + "; ".join(problems)))
    if verbose or not ok:
        for l in out:
            print("        " + l)
    return ok


def main() -> int:
    try:
        sys.stdout.reconfigure(errors="replace")
    except AttributeError:
        pass
    verbose = "-v" in sys.argv[1:]
    files = sorted(FIX.glob("pass_*.yaml")) + sorted(FIX.glob("fail_*.yaml"))
    if not files:
        print("no fixtures found")
        return 1
    results = [run_one(f, verbose) for f in files]
    nfail = results.count(False)
    print(f"\n{len(results) - nfail}/{len(results)} passed")
    return 1 if nfail else 0


if __name__ == "__main__":
    sys.exit(main())
