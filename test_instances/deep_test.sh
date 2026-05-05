#!/bin/bash
# Deep release-gate test for minion. Runs a layered sequence of
# correctness checks at progressively larger instance sizes and trial
# counts. Designed for an unattended pre-release run on a workstation;
# total budget defaults to 48 hours, with each phase honouring the
# remaining budget so a slow phase doesn't starve later ones.
#
# Usage:
#   ./deep_test.sh <minion_binary> [--budget-hours N | --budget-minutes M]
#                                  [--logdir DIR]
#
# Examples:
#   ./deep_test.sh ../bin-pp/minion --budget-hours 24
#   ./deep_test.sh ../bin-pp/minion --budget-minutes 90   # quick smoke
#
# Output: per-phase log files plus a final summary in $LOGDIR.
# Exit code is non-zero if any phase failed; the summary lists which.
#
# What each phase exercises:
#   1. .minion regression suite (do_basic_tests + random heuristics)
#   2. minion-sys FFI integration tests (test_work_steal, parallel, ...)
#   3. tester baseline at default size — high-trial smoke
#   4. tester deep-nested cross-type parent trees (depth=3)
#   5. tester size_factor=2 with high counts
#   6. tester size_factor=4 (longer wall-clock, exercises ws donation)
#   7. tester in-process backend (libminion FFI) baseline
#   8. tester in-process mid-search constraint injection
#   9. per-constraint isolation sweep (each constraint in its own
#      tester process, 5 in parallel — crash isolation)
#  10. tester size_factor=8 (very long; only if budget permits)
#  11. metamorphic optimisation sweep (5 strategies must agree on
#      optimum; catches bound-tracking / parallel-bound-broadcast
#      bugs)
#  12. AddressSanitizer build + regression suite + tester baseline
#      (catches heap UB / use-after-free / stack overflow that
#      _GLIBCXX_DEBUG misses)

set -u
set -o pipefail

# --- arg parsing ---

if [ $# -lt 1 ]; then
  echo "Usage: $0 <minion_binary> [--budget-hours N | --budget-minutes M] [--logdir DIR]" >&2
  exit 2
fi

MINION="$1"
shift

BUDGET_MINUTES=$((48 * 60))
LOGDIR=""

while [ $# -gt 0 ]; do
  case "$1" in
    --budget-hours)
      # Integer hours only (bash can't do float arithmetic). For
      # finer control use --budget-minutes.
      BUDGET_MINUTES=$(($2 * 60)); shift 2 ;;
    --budget-minutes)
      BUDGET_MINUTES="$2"; shift 2 ;;
    --logdir)
      LOGDIR="$2"; shift 2 ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2 ;;
  esac
done

if [ ! -x "$MINION" ]; then
  echo "Error: minion binary $MINION is not executable" >&2
  exit 2
fi

# Resolve absolute paths so child phases work regardless of cwd.
MINION=$(cd "$(dirname "$MINION")" && pwd)/$(basename "$MINION")
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
TESTER_DIR="$REPO_DIR/tester"
MINIONSYS_DIR="$REPO_DIR/minion-sys"

if [ -z "$LOGDIR" ]; then
  TS=$(date +%Y%m%d-%H%M%S)
  LOGDIR="/tmp/minion-deep-$TS"
fi
mkdir -p "$LOGDIR"

START_EPOCH=$(date +%s)
BUDGET_SECONDS=$((BUDGET_MINUTES * 60))
DEADLINE_EPOCH=$((START_EPOCH + BUDGET_SECONDS))

# Track phase results for the final summary.
PHASES=()
RESULTS=()
ELAPSEDS=()
OVERALL_RC=0

# --- helpers ---

now_iso() { date '+%Y-%m-%d %H:%M:%S'; }

remaining_seconds() {
  echo $((DEADLINE_EPOCH - $(date +%s)))
}

# Per-phase wrapper: sets up a log, prints start/end markers, captures
# elapsed time and exit code. Doesn't abort on failure — we want the
# full picture, and a single phase failing shouldn't mask later phases.
#
# Args: <phase-id> <phase-name> <command...>
run_phase() {
  local id="$1"; shift
  local name="$1"; shift
  local log="$LOGDIR/$(printf '%02d' "$id")-$(echo "$name" | tr ' /' '_-').log"

  echo
  echo "=== [$id] $name ===" | tee -a "$LOGDIR/00-progress.log"
  echo "  start: $(now_iso)  budget remaining: $(($(remaining_seconds)/60))min" \
    | tee -a "$LOGDIR/00-progress.log"
  echo "  log:   $log" | tee -a "$LOGDIR/00-progress.log"

  local started; started=$(date +%s)
  # stdbuf forces line-buffered stdout/stderr so the log is readable
  # while the phase runs (per CLAUDE.md: long-running experiments
  # must flush explicitly). On macOS with homebrew, stdbuf may not be
  # in $PATH; fall back to running the command directly.
  if command -v stdbuf >/dev/null 2>&1; then
    stdbuf -oL -eL "$@" >"$log" 2>&1
  else
    "$@" >"$log" 2>&1
  fi
  local rc=$?
  local ended; ended=$(date +%s)
  local elapsed=$((ended - started))

  PHASES+=("$id $name")
  RESULTS+=("$rc")
  ELAPSEDS+=("$elapsed")

  if [ "$rc" -eq 0 ]; then
    echo "  OK   ($((elapsed/60))m $((elapsed%60))s)" \
      | tee -a "$LOGDIR/00-progress.log"
  else
    echo "  FAIL rc=$rc ($((elapsed/60))m $((elapsed%60))s) — see $log" \
      | tee -a "$LOGDIR/00-progress.log"
    OVERALL_RC=1
  fi
}

# Skip a phase if less than the requested headroom remains. Phases
# are ordered roughly by importance, so skipping later ones when the
# budget runs out is the right behaviour.
skip_if_no_budget() {
  local needed_min="$1"
  local remaining_min=$(($(remaining_seconds) / 60))
  if [ "$remaining_min" -lt "$needed_min" ]; then
    echo
    echo "  skip: only ${remaining_min}min remaining, phase needs ~${needed_min}min" \
      | tee -a "$LOGDIR/00-progress.log"
    return 1
  fi
  return 0
}

# --- preamble ---

echo "Deep test starting at $(now_iso)" | tee -a "$LOGDIR/00-progress.log"
echo "  minion:   $MINION" | tee -a "$LOGDIR/00-progress.log"
echo "  budget:   ${BUDGET_MINUTES}min (deadline $(date -r $DEADLINE_EPOCH '+%Y-%m-%d %H:%M:%S' 2>/dev/null || date -d "@$DEADLINE_EPOCH" '+%Y-%m-%d %H:%M:%S'))" \
  | tee -a "$LOGDIR/00-progress.log"
echo "  logdir:   $LOGDIR" | tee -a "$LOGDIR/00-progress.log"
echo "  rev:      $(cd "$REPO_DIR" && git rev-parse --short HEAD 2>/dev/null || echo unknown)" \
  | tee -a "$LOGDIR/00-progress.log"

# --- phases ---

# Phase 1: .minion regression suite (~5-15 min depending on machine).
if skip_if_no_budget 5; then
  run_phase 1 "minion-regression-suite" \
    bash -c "cd '$REPO_DIR/test_instances' && ./run_tests.sh '$MINION'"
fi

# Phase 2: minion-sys FFI integration tests (~5 min).
if skip_if_no_budget 5; then
  run_phase 2 "minion-sys-cargo-test" \
    bash -c "cd '$MINIONSYS_DIR' && cargo test --release"
fi

# Phase 3: tester baseline at default size, high trial count
# (~30 min). Covers basic, nested, parallel, work-steal, restart,
# variant-equivalence, and option sweeps at size_factor=1.
if skip_if_no_budget 20; then
  run_phase 3 "tester-baseline" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --minion '$MINION' \
      --count 500 --variant-count 500 --optioncount 5000 \
      --numthreads 4"
fi

# Phase 4: deep-nested cross-type parent trees (depth-3).
# This stress-tests parent-constraint state interactions
# (reify(watched-or(reifyimply(X), watched-and(Y,Z)))-shaped trees).
if skip_if_no_budget 20; then
  run_phase 4 "tester-deep-nest" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --minion '$MINION' \
      --count 100 --variant-count 100 --optioncount 0 \
      --nest-depth 3 --numthreads 4"
fi

# Phase 5: size_factor=2 with high counts (~1 h).
# Bigger random instances exercise propagator code paths that the
# defaults rarely hit (wider lists, larger domains).
if skip_if_no_budget 30; then
  run_phase 5 "tester-size2" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --minion '$MINION' \
      --count 200 --variant-count 200 --optioncount 1000 \
      --size-factor 2 --ws-max-size 64 --numthreads 4"
fi

# Phase 6: size_factor=4 (~2 h). Crucial for actually exercising the
# work-stealing donation/replay path — at size_factor=1 most
# constraints finish before workers mark themselves idle.
if skip_if_no_budget 60; then
  run_phase 6 "tester-size4" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --minion '$MINION' \
      --count 100 --variant-count 100 --optioncount 500 \
      --size-factor 4 --ws-max-size 128 --numthreads 4"
fi

# Phase 7: in-process backend baseline. Drives libminion via the FFI
# entry points — different code path from exec mode (no subprocess
# isolation, shares minion-sys's memory model).
if skip_if_no_budget 10; then
  run_phase 7 "tester-inproc-baseline" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --in-process --count 100 --variant-count 100 --numthreads 4"
fi

# Phase 8: in-process mid-search constraint injection. Multi-packet
# variant exercises mid-search ParentConstraint state setup, which
# has had a series of subtle bugs.
if skip_if_no_budget 30; then
  run_phase 8 "tester-inproc-midsearch" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --in-process --midsearch-constraints \
      --midsearch-constraints-num-packets 3 \
      --count 500 --numthreads 4"
fi

# Phase 9: per-constraint isolation sweep. Each leaf constraint runs
# in its own tester process so a crash on one constraint doesn't kill
# the others. Higher per-constraint trial count than the in-tester
# parallel sweep can reach.
#
# The big-test-logs runner has its own internal timeout and parallel
# execution, but it hardcodes its own paths — we invoke it via env
# overrides and let it log into our logdir.
if skip_if_no_budget 240; then
  run_phase 9 "tester-per-constraint-isolation" \
    bash -c "cd '$TESTER_DIR' && \
      MINION='$MINION' LOGDIR='$LOGDIR/per-constraint' \
      COUNT=500 OPTIONCOUNT=200 NUMTHREADS=2 \
      WS_MAX_SIZE=64 PER_CONSTRAINT_TIMEOUT=3600 JOBS=4 \
      bash big-test-logs/run.sh"
fi

# Phase 10: size_factor=8. Very large instances; only attempt if at
# least 4 hours remain. Heavy reliance on --max-solutions cap-skip
# for trials whose Cartesian product blows out memory.
if skip_if_no_budget 240; then
  run_phase 10 "tester-size8" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --minion '$MINION' \
      --count 50 --variant-count 50 --optioncount 200 \
      --size-factor 8 --ws-max-size 256 --numthreads 4"
fi

# Phase 11: metamorphic optimisation sweep. For every constraint,
# wrap it with an aux objective and check five strategies (baseline,
# SAC preprocess, var-sdf-val-desc, work-steal 4, threads 4) all
# agree on the optimum. Catches bound-tracking and parallel-bound-
# broadcast bugs that the satisfaction sweeps miss.
if skip_if_no_budget 60; then
  run_phase 11 "tester-optimisation-sweep" \
    bash -c "cd '$TESTER_DIR' && cargo run --release -- \
      --minion '$MINION' \
      --optimisation-sweep --count 200 --numthreads 4"
fi

# Phase 12: AddressSanitizer pass. Builds a separate bin-asan/
# binary with -fsanitize=address (clang) and runs the .minion
# regression suite plus a moderate tester sweep under it. Catches
# heap UB, use-after-free, and stack-overflow bugs that the
# debug build's _GLIBCXX_DEBUG misses.
#
# ASan adds ~2x runtime overhead and ~3x memory; the trial counts
# below are scaled down so this phase fits in roughly an hour.
# Skips cleanly if clang isn't available — configure.py forces
# clang++ when --sanitize is set, so a pure-gcc system won't have
# it. Also skips on architectures where ASan isn't supported (the
# build will fail and the phase will report rc!=0).
if skip_if_no_budget 30 && command -v clang++ >/dev/null 2>&1; then
  ASAN_DIR="$REPO_DIR/bin-asan"
  if [ ! -x "$ASAN_DIR/minion" ]; then
    run_phase 12 "asan-build" \
      bash -c "mkdir -p '$ASAN_DIR' && cd '$ASAN_DIR' && \
        python3 '$REPO_DIR/configure.py' --sanitize && make -j4"
  fi
  if [ -x "$ASAN_DIR/minion" ]; then
    run_phase 13 "asan-regression-suite" \
      bash -c "cd '$REPO_DIR/test_instances' && ./run_tests.sh '$ASAN_DIR/minion'"
    run_phase 14 "asan-tester-baseline" \
      bash -c "cd '$TESTER_DIR' && cargo run --release -- \
        --minion '$ASAN_DIR/minion' \
        --count 30 --variant-count 30 --optioncount 200 --numthreads 2"
  fi
fi

# --- summary ---

TOTAL=$(($(date +%s) - START_EPOCH))
{
  echo
  echo "=== Deep test summary ==="
  echo "  finished: $(now_iso)"
  echo "  elapsed:  $((TOTAL/3600))h $((TOTAL/60%60))m $((TOTAL%60))s"
  echo "  logdir:   $LOGDIR"
  echo
  printf "  %-44s %-6s %s\n" "phase" "rc" "elapsed"
  for i in "${!PHASES[@]}"; do
    local_phase="${PHASES[$i]}"
    local_rc="${RESULTS[$i]}"
    local_el="${ELAPSEDS[$i]}"
    printf "  %-44s %-6s %dm%ds\n" \
      "$local_phase" "$local_rc" $((local_el/60)) $((local_el%60))
  done
  echo
  if [ "$OVERALL_RC" -eq 0 ]; then
    echo "Result: PASS"
  else
    echo "Result: FAIL — see per-phase logs in $LOGDIR"
  fi
} | tee "$LOGDIR/99-summary.log"

exit "$OVERALL_RC"
