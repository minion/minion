#!/usr/bin/env python3
"""Run the .minion regression suite without a POSIX shell.

do_basic_tests.sh is the reference implementation and remains the one
used on Linux and macOS. This exists because minion.exe cannot be run
from git-bash on the Windows CI runners: every invocation dies with exit
139 before producing output, whatever the working directory or instance,
and whether bash starts it directly or by way of `cmd /c`. The same
binary run from cmd solves everything correctly. So the Windows job
drives this from cmd instead, keeping MSYS out of the chain entirely.

Supports the same directives as do_basic_tests.sh:
  #TEST SOLCOUNT n       -findallsols, compare "Solutions Found:"
  #TEST CHECKONESOL ...  compare the first solution
  #TEST NODECOUNT n      compare "Total Nodes:"
  #TEST EXITCODE1        minion must exit 1
  #TEST EXTRAFLAGS ...   extra flags for this instance
  #FAIL                  the test is expected to fail
  #BUG                   known-broken: failing is expected, passing is not

Exit status is the number of unexpected failures, as in the shell
version.
"""
import re
import subprocess
import sys
from pathlib import Path


def run(exe, cwd, args):
    p = subprocess.run([exe, *args], capture_output=True, text=True, cwd=cwd)
    return p.stdout, p.returncode


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


def check(exe, path):
    """(passed, message) for one instance. #FAIL/#BUG are the caller's."""
    text = path.read_text(errors="replace")
    cwd = str(path.parent)
    name = path.name
    extra = []
    m = re.search(r"^#TEST EXTRAFLAGS(.*)$", text, re.M)
    if m:
        extra = m.group(1).split()

    m = re.search(r"^#TEST SOLCOUNT\s+(\S+)", text, re.M)
    if m:
        out, _ = run(exe, cwd, [name, *extra, "-findallsols"])
        got, want = field(out, "Solutions Found:"), m.group(1)
        return got == want, f"Got '{got}' instead of '{want}' solutions in {name}"

    m = re.search(r"^#TEST CHECKONESOL(.*)$", text, re.M)
    if m:
        out, _ = run(exe, cwd, [name, *extra])
        got, want = first_solution(out), norm(m.group(1))
        return got == want, f"Got '{got}' instead of '{want}' as solution in {name}"

    m = re.search(r"^#TEST NODECOUNT\s+(\S+)", text, re.M)
    if m:
        out, _ = run(exe, cwd, [name, *extra])
        got, want = field(out, "Total Nodes:"), m.group(1)
        return got == want, f"Got '{got}' instead of '{want}' search nodes in {name}"

    if "#TEST EXITCODE1" in text:
        _, rc = run(exe, cwd, [name, *extra])
        return rc == 1, f"Got return code of {rc}, expected 1 in {name}"

    print(f"Test {name} is not well-formed.")
    sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print("Must give a minion binary to test.")
        return 2
    exe = Path(sys.argv[1]).resolve()
    if not exe.is_file():
        print(f"{exe} does not exist.")
        return 2

    # Directory of instances; defaults to this script's own, which is
    # what CI uses. Overridable so the runner itself can be tested.
    here = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else Path(__file__).resolve().parent
    instances = sorted(here.glob("*.minion"))

    npass = expected_fail = unexpected_pass = 0
    for path in instances:
        print(".", end="", flush=True)
        text = path.read_text(errors="replace")
        want_pass = "#FAIL" not in text
        buggy = "#BUG" in text

        passed, message = check(str(exe), path)

        if not buggy:
            if passed == want_pass:
                npass += 1
            elif not want_pass:
                print(f"\nExpected {path.name} to fail.")
            else:
                print(f"\n{message}")
        else:
            if passed == want_pass:
                print(f"\n{path.name} passed, but is supposed to be buggy!")
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


if __name__ == "__main__":
    sys.exit(main())
