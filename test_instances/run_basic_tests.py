#!/usr/bin/env python3
"""Per-instance checks for run_tests.py, also runnable on their own.

Directives:
  #TEST SOLCOUNT n       -findallsols, compare "Solutions Found:"
  #TEST CHECKONESOL ...  compare the first solution
  #TEST NODECOUNT n      compare "Total Nodes:"
  #TEST EXITCODE1        minion must exit 1
  #TEST EXITCODE0        minion must exit 0
  #TEST EXITCODE9        minion must reject input via its exception handler
  #TEST EXTRAFLAGS ...   extra flags for this instance
  #FAIL                  the test is expected to fail
  #BUG                   known-broken: failing is expected, passing is not

Exit status is non-zero if any test fails unexpectedly.
"""
import argparse
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def diagnostics(p):
    return (f"Command: {p.args!r}\nExit code: {p.returncode}"
            f"\nstdout:\n{p.stdout}\nstderr:\n{p.stderr}")


class Solver:
    def __init__(self, exe, timeout=60, budget_seconds=900):
        self.exe = str(Path(exe).resolve())
        if not Path(self.exe).is_file():
            raise ValueError(f"{self.exe} does not exist")
        if timeout <= 0 or budget_seconds <= 0:
            raise ValueError("Timeout and suite budget must be positive")
        self.timeout = timeout
        self.deadline = time.monotonic() + budget_seconds

    def run(self, cwd, args, expected_exit=0):
        command = [self.exe, *args]
        remaining = self.deadline - time.monotonic()
        if remaining <= 0:
            raise RuntimeError(f"Suite budget exhausted before command: {command!r}")
        try:
            p = subprocess.run(command, capture_output=True, text=True, cwd=cwd,
                               timeout=min(self.timeout, remaining))
        except subprocess.TimeoutExpired as error:
            stdout = error.stdout or b""
            stderr = error.stderr or b""
            if isinstance(stdout, bytes):
                stdout = stdout.decode(errors="replace")
            if isinstance(stderr, bytes):
                stderr = stderr.decode(errors="replace")
            raise RuntimeError(f"Command: {command!r}\nTimed out after {error.timeout}s"
                               f"\nstdout:\n{stdout}\nstderr:\n{stderr}") from error
        # Exit 1 is Minion's normal input-error status, explicitly expected
        # by negative tests. Signals and other statuses must never be
        # accepted by #FAIL or #BUG.
        if p.returncode not in (0, 1, expected_exit):
            raise RuntimeError(diagnostics(p))
        return p


def result(p, passed, message):
    return passed, message + "\n" + diagnostics(p)


def field(out, label):
    m = re.search(rf"^{re.escape(label)}\s*(\S+)", out, re.M)
    return m.group(1) if m else None


def first_solution(out):
    # Minion prints a "Sol: " line per print block; join the values the
    # way print_sol.sh does so comparisons match the shell version.
    vals = []
    for line in out.splitlines():
        if line.startswith("Sol: "):
            vals.extend(line[5:].split())
    return ",".join(vals)


def norm(s):
    return ",".join(s.split())


def extra_flags(text):
    m = re.search(r"^#TEST EXTRAFLAGS(.*)$", text, re.M)
    return m.group(1).split() if m else []


def check(solver, path, extra_args=()):
    """(passed, message) for one instance. #FAIL/#BUG are the caller's."""
    text = path.read_text(errors="replace")
    cwd = str(path.parent)
    name = path.name
    extra = [*extra_flags(text), *extra_args]

    m = re.search(r"^#TEST SOLCOUNT\s+(\S+)", text, re.M)
    if m:
        p = solver.run(cwd, [name, *extra, "-findallsols"])
        got, want = field(p.stdout, "Solutions Found:"), m.group(1)
        return result(p, p.returncode == 0 and got == want,
                      f"Got '{got}' instead of '{want}' solutions in {name}")

    m = re.search(r"^#TEST CHECKONESOL(.*)$", text, re.M)
    if m:
        p = solver.run(cwd, [name, *extra])
        got, want = first_solution(p.stdout), norm(m.group(1))
        return result(p, p.returncode == 0 and got == want,
                      f"Got '{got}' instead of '{want}' as solution in {name}")

    m = re.search(r"^#TEST NODECOUNT\s+(\S+)", text, re.M)
    if m:
        p = solver.run(cwd, [name, *extra])
        got, want = field(p.stdout, "Total Nodes:"), m.group(1)
        return result(p, p.returncode == 0 and got == want,
                      f"Got '{got}' instead of '{want}' search nodes in {name}")

    m = re.search(r"^#TEST EXITCODE([019])\s*$", text, re.M)
    if m:
        expected = int(m.group(1))
        p = solver.run(cwd, [name, *extra], expected_exit=expected)
        return result(p, p.returncode == expected,
                      f"Got return code of {p.returncode}, expected {expected} in {name}")

    print(f"Test {name} is not well-formed.")
    sys.exit(1)


def run_suite(solver, directory=HERE, extra_args=()):
    here = Path(directory).resolve()
    instances = sorted(here.glob("*.minion"))
    if not instances:
        raise ValueError(f"No .minion instances in {here}")

    npass = expected_fail = unexpected_pass = 0
    for path in instances:
        print(".", end="", flush=True)
        text = path.read_text(errors="replace")
        want_pass = "#FAIL" not in text
        buggy = "#BUG" in text

        passed, message = check(solver, path, extra_args)

        if not buggy:
            if passed == want_pass:
                npass += 1
            elif not want_pass:
                print(f"\nExpected {path.name} to fail.\n{message}", flush=True)
            else:
                print(f"\n{message}", flush=True)
        else:
            if passed == want_pass:
                print(f"\n{path.name} passed, but is supposed to be buggy!\n{message}", flush=True)
                unexpected_pass += 1
            else:
                expected_fail += 1

    total = len(instances)
    failed = total - npass - expected_fail
    print()
    print(f"{npass} of {total} tests successful.")
    print(f"{failed} tests failed due to unexpected errors.")
    print(f"{expected_fail} tests failed due to expected errors.")
    print(f"{unexpected_pass} tests passed unexpectedly.")
    return failed


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("binary")
    parser.add_argument("directory", nargs="?", default=HERE)
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--budget-seconds", type=float, default=900)
    options = parser.parse_args()
    solver = Solver(options.binary, options.timeout, options.budget_seconds)
    return int(run_suite(solver, options.directory) != 0)


if __name__ == "__main__":
    sys.exit(main())
