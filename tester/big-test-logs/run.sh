#!/bin/bash
# Big stress test: every leaf constraint in its own tester process,
# 5 in parallel, ~6h total budget. Process isolation means a crash
# in one constraint doesn't kill the others. Each per-constraint run
# is hard-capped at 90 minutes so a hang doesn't eat the budget.
#
# Usage: bash run.sh
#
# Output: one log file per constraint under $LOGDIR, plus a summary
# at the end. The summary records exit code and the last useful
# lines of stderr/stdout for each FAILED or TIMED-OUT run.

set -u
set -o pipefail

# Defaults can be overridden by env vars from a parent script
# (deep_test.sh in particular drives this with its own paths).
REPO_DEFAULT=$(cd "$(dirname "$0")/../.." && pwd)
REPO="${REPO:-$REPO_DEFAULT}"
TESTER="${TESTER:-$REPO/tester/target/release/tester}"
MINION="${MINION:-$REPO/bin-ws/minion}"
TS=$(date +%Y%m%d-%H%M%S)
LOGDIR="${LOGDIR:-$REPO/tester/big-test-logs/$TS}"
mkdir -p "$LOGDIR"

# How long any single tester process is allowed to run, in seconds.
PER_CONSTRAINT_TIMEOUT="${PER_CONSTRAINT_TIMEOUT:-5400}"   # 90 minutes
# How many constraints to run in parallel. Each tester process spawns
# 4 internal rayon threads, so 5×4 = 20 concurrent solves on a 10-core
# machine — moderate over-subscription, fine for stressing parallel
# code paths.
JOBS="${JOBS:-5}"
# Per-constraint workload. Tuned so the whole sweep fits roughly within
# 6 hours wall time at JOBS=5: 64 constraints × ~30 min average / 5 ≈
# 6h 24min worst case, less in practice because most constraints are
# fast.
COUNT="${COUNT:-1000}"
OPTIONCOUNT="${OPTIONCOUNT:-200}"
NUMTHREADS="${NUMTHREADS:-4}"
WS_MAX_SIZE="${WS_MAX_SIZE:-64}"

CONSTRAINTS=(
  abs alldiff alldiffmatrix difference diseq div div_undefzero
  element element_one element_undefzero eq false gacalldiff gaceq
  gacschema gcc gccweak hamming ineq lexleq 'lexleq[quick]'
  'lexleq[rv]' lexless 'lexless[quick]' lighttable litsumgeq max
  mddc min minuseq modulo modulo_undefzero negativemddc negativetable
  not-hamming nvaluegeq nvalueleq occurrence occurrencegeq
  occurrenceleq pow product sumgeq sumleq table true w-inintervalset
  w-inrange w-inset w-literal w-notinrange w-notinset w-notliteral
  watchelement watchelement_one watchelement_one_undefzero
  watchelement_undefzero watchless watchneq watchsumgeq watchsumleq
  watchvecexists_less watchvecneq weightedsumgeq weightedsumleq
)

echo "Starting big test sweep" | tee "$LOGDIR/00-meta.log"
echo "  ts: $TS" | tee -a "$LOGDIR/00-meta.log"
echo "  logdir: $LOGDIR" | tee -a "$LOGDIR/00-meta.log"
echo "  constraints: ${#CONSTRAINTS[@]}" | tee -a "$LOGDIR/00-meta.log"
echo "  jobs: $JOBS" | tee -a "$LOGDIR/00-meta.log"
echo "  per-constraint timeout: ${PER_CONSTRAINT_TIMEOUT}s" | tee -a "$LOGDIR/00-meta.log"
echo "  count=$COUNT optioncount=$OPTIONCOUNT numthreads=$NUMTHREADS ws_max_size=$WS_MAX_SIZE" | tee -a "$LOGDIR/00-meta.log"
echo "  minion: $(cd $REPO && git rev-parse --short HEAD)" | tee -a "$LOGDIR/00-meta.log"
echo >> "$LOGDIR/00-meta.log"

# Sanitise constraint name for use as a filename. Replace
# characters that are awkward in shell paths (brackets) with `_`.
sanitise() { echo "$1" | tr '[]' '__'; }

run_one() {
  local c="$1"
  local fname; fname=$(sanitise "$c")
  local log="$LOGDIR/$fname.log"
  local rc_file="$LOGDIR/$fname.exit"
  local started ended
  started=$(date +%s)
  echo "[$(date +%H:%M:%S)] start: $c" >&2
  # stdbuf -oL -eL flushes per-line so the running log file is
  # readable while the process is still running. This is important —
  # the user's CLAUDE.md requires explicit flushing on long-running
  # experiments. Skip the wrapper if stdbuf isn't installed (rare on
  # Linux, occasional on minimal macOS without homebrew coreutils).
  STDBUF=$(command -v stdbuf || true)
  ${STDBUF:+$STDBUF -oL -eL} \
    timeout --foreground --kill-after=30s "${PER_CONSTRAINT_TIMEOUT}s" \
    "$TESTER" \
      --minion "$MINION" \
      --count "$COUNT" \
      --optioncount "$OPTIONCOUNT" \
      --numthreads "$NUMTHREADS" \
      --ws-max-size "$WS_MAX_SIZE" \
      --constraints "$c" \
    >"$log" 2>&1
  local rc=$?
  ended=$(date +%s)
  echo "$rc" > "$rc_file"
  local elapsed=$((ended - started))
  if [ "$rc" -eq 0 ]; then
    echo "[$(date +%H:%M:%S)] OK ($elapsed s): $c" >&2
  elif [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
    echo "[$(date +%H:%M:%S)] TIMEOUT ($elapsed s): $c" >&2
  else
    echo "[$(date +%H:%M:%S)] FAIL rc=$rc ($elapsed s): $c" >&2
  fi
}

export -f run_one sanitise
export TESTER MINION LOGDIR PER_CONSTRAINT_TIMEOUT COUNT OPTIONCOUNT NUMTHREADS WS_MAX_SIZE

START=$(date +%s)

printf '%s\n' "${CONSTRAINTS[@]}" \
  | xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {}

END=$(date +%s)
TOTAL=$((END - START))

# Summary.
{
  echo
  echo "=== Summary ($((TOTAL/60))m $((TOTAL%60))s wall) ==="
  ok=0; fail=0; tmo=0
  for c in "${CONSTRAINTS[@]}"; do
    fname=$(sanitise "$c")
    rc=$(cat "$LOGDIR/$fname.exit" 2>/dev/null || echo "?")
    case "$rc" in
      0)        ok=$((ok+1));;
      124|137)  tmo=$((tmo+1)); echo "TIMEOUT: $c";;
      "?")      fail=$((fail+1)); echo "MISSING: $c (no exit file)";;
      *)        fail=$((fail+1)); echo "FAIL rc=$rc: $c";;
    esac
  done
  echo
  echo "ok=$ok fail=$fail timeout=$tmo total=${#CONSTRAINTS[@]}"
} | tee "$LOGDIR/99-summary.log"

# For any failure, print the last error context.
{
  echo
  echo "=== Failure tails ==="
  for c in "${CONSTRAINTS[@]}"; do
    fname=$(sanitise "$c")
    rc=$(cat "$LOGDIR/$fname.exit" 2>/dev/null || echo "?")
    if [ "$rc" != "0" ]; then
      echo
      echo "--- $c (rc=$rc) ---"
      tail -20 "$LOGDIR/$fname.log" 2>/dev/null || echo "(no log)"
    fi
  done
} | tee -a "$LOGDIR/99-summary.log"
