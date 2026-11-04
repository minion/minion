#!/usr/bin/env python3
"""Full .minion regression suite: basic, randomised, and special tests.

Runs directly through Python on every platform, including Windows.

Usage:
    run_tests.py <minion binary> [options] [extra minion flags...]

Options:
    --big           also cover tests-64domains/
    --random-only   just the randomised sweeps, which are the ones safe
                    under randomised orderings and parallel exploration
    --basic-only    just the per-instance checks
    --timeout N     maximum seconds per solver invocation (default 60)
    --budget-seconds N  overall suite budget (default 900)
"""
import argparse
import re
import sys
from pathlib import Path

import run_basic_tests as basic

HERE = Path(__file__).resolve().parent

# (repeats, flags) for the randomised sweep.
# Deterministic orderings need one pass; randomised ones get three.
RANDOM_SWEEPS = [
    (3, ["-randomiseorder"]),
    (3, ["-varorder", "random"]),
    (1, ["-varorder", "static"]),
    (1, ["-varorder", "sdf"]),
    (3, ["-varorder", "sdf-random"]),
    (1, ["-varorder", "srf"]),
    (3, ["-varorder", "srf-random"]),
    (1, ["-varorder", "ldf"]),
    (3, ["-varorder", "ldf-random"]),
]
# Only meaningful when minion was built with wdeg.
WDEG_SWEEPS = [
    (3, ["-varorder", "wdeg"]),
    (3, ["-varorder", "domoverwdeg"]),
]

# Instances whose solution count is not stable under a different search
# order: known failures, known bugs, and optimisation problems (which
# report the optimum rather than enumerating).
SKIP_RANDOM = re.compile(r"#FAIL|#BUG|maximising|minimising|MAXIMISING|MINIMISING")


def random_candidates(big=False):
    """Instances usable in the randomised sweep, in the same order the
    shell version walked them."""
    out = []
    patterns = ["*.minion", "tests-32domains/*.minion"]
    if big:
        patterns.append("tests-64domains/*.minion")
    for pattern in patterns:
        for path in sorted(HERE.glob(pattern)):
            text = path.read_text(errors="replace")
            if "#TEST SOLCOUNT" not in text:
                continue
            if SKIP_RANDOM.search(text):
                continue
            out.append(path)
    return out


def random_sweep(solver, repeats, flags, extra, big=False):
    """Re-solve each instance under `repeats` seeds; the solution count
    must not depend on the search order."""
    print(f"Random testing {solver.exe} with options .{' '.join(flags + list(extra))}.", flush=True)
    npass = total = 0
    candidates = random_candidates(big)
    if not candidates:
        raise ValueError(f"No randomised test candidates in {HERE}")
    for path in candidates:
        text = path.read_text(errors="replace")
        want = re.search(r"^#TEST SOLCOUNT\s+(\S+)", text, re.M).group(1)
        for seed in range(repeats):
            total += 1
            p = solver.run(
                str(path.parent),
                [path.name, *basic.extra_flags(text), "-randomseed", str(seed),
                 "-findallsols", *flags, *extra],
            )
            got = basic.field(p.stdout, "Solutions Found:")
            if p.returncode != 0 or got != want:
                print(f"\nGot {got} instead of {want} solutions in {path.name}"
                      f"\n{basic.diagnostics(p)}", flush=True)
            else:
                npass += 1
        print(".", end="", flush=True)
    print()
    print(f"{npass} of {total} randomised tests successful.")
    return total - npass


def has_wdeg(solver):
    p = solver.run(str(HERE), ["--help", "varorder"])
    if p.returncode != 0:
        raise RuntimeError(basic.diagnostics(p))
    m = re.search(r"this one is (such a build|not)\.", p.stdout)
    if not m:
        raise RuntimeError("Missing wdeg build information\n" + basic.diagnostics(p))
    return m.group(1) == "such a build"


def special_tests(solver):
    """Preprocessing counts and the -X-AMO output, checked against
    values recorded when those features were written."""
    d = HERE / "special_tests"
    failures = 0

    for level, label, want in (("SAC", "SAC", "36"),
                               ("SSAC", "SAC", "36"),
                               ("SSAC", "SSAC", "244")):
        p = solver.run(str(d), ["bibd.minion", "-preprocess", level])
        m = re.search(rf"^{label}\s+\S+\s+(\S+)", p.stdout, re.M)
        got = m.group(1) if m else None
        if p.returncode != 0 or got != want:
            print(f"{level} test ({label}) failed: got {got}, expected {want}"
                  f"\n{basic.diagnostics(p)}", flush=True)
            failures += 1

    p = solver.run(str(d), ["bibd.minion", "-X-AMO",
                                     "-preprocess", "SACBounds_limit"])
    got = [l for l in p.stdout.splitlines()
           if ("BOOLNAMES" in l or "AMO" in l) and "Command" not in l]
    want = (d / "bibd.minion-amo").read_text(errors="replace").splitlines()
    if p.returncode != 0 or got != want:
        print(f"AMO test bibd.minion failed\nExpected: {want!r}\nGot: {got!r}"
              f"\n{basic.diagnostics(p)}", flush=True)
        failures += 1

    print(f"{4 - failures} of 4 special checks successful.", flush=True)
    return failures


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("binary")
    parser.add_argument("--big", action="store_true")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--random-only", action="store_true")
    mode.add_argument("--basic-only", action="store_true")
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--budget-seconds", type=float, default=900)
    options, extra = parser.parse_known_args(argv[1:])
    solver = basic.Solver(options.binary, options.timeout, options.budget_seconds)
    big, random_only = options.big, options.random_only

    print(f"Testing {solver.exe} with options .{' '.join(extra)}.", flush=True)
    failed = 0
    if not random_only:
        failed += basic.run_suite(solver, str(HERE), extra)
        if big:
            failed += basic.run_suite(solver, str(HERE / "tests-64domains"), extra)

    if options.basic_only or failed:
        return int(failed != 0)

    sweeps = RANDOM_SWEEPS + (WDEG_SWEEPS if has_wdeg(solver) else [])
    for repeats, flags in sweeps:
        failed += random_sweep(solver, repeats, flags, extra, big)

    # Bail before the special tests if anything above failed.
    if failed > 0:
        return 1
    if random_only:
        return 0

    return int(special_tests(solver) != 0)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
