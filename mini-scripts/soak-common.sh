# Shared helpers for the release soak scripts. Sourced, not executed.
#
# The soaks are budget-driven, not count-driven: each runs rounds of
# increasing difficulty until its time is up. A trial count tuned on one
# machine means nothing on another; a budget does.

set -u
set -o pipefail

SOAK_REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SOAK_TESTER_DIR="$SOAK_REPO/tester"
SOAK_TESTER="$SOAK_TESTER_DIR/target/release/tester"

# --- portability -----------------------------------------------------

# macOS has no `timeout` unless coreutils is installed, where it is
# `gtimeout`. A soak without per-item timeouts is not safe to leave
# running unattended, so refuse to start rather than silently drop them.
SOAK_TIMEOUT=$(command -v timeout || command -v gtimeout || true)
SOAK_STDBUF=$(command -v stdbuf || command -v gstdbuf || true)

soak_require_tools() {
  if [ -z "$SOAK_TIMEOUT" ]; then
    echo "error: neither 'timeout' nor 'gtimeout' found." >&2
    echo "  These soaks run for days unattended and rely on per-item" >&2
    echo "  timeouts to survive a hung trial. Install GNU coreutils" >&2
    echo "  (macOS: brew install coreutils) and re-run." >&2
    exit 2
  fi
}

# --- argument parsing ------------------------------------------------

SOAK_BUDGET_MINUTES=""
SOAK_JOBS=""
SOAK_LOGDIR=""
SOAK_NUMTHREADS=4
SOAK_SMOKE=0

soak_nproc() { getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4; }

soak_parse_args() {
  # soak_parse_args <default-budget-minutes> "$@"
  SOAK_BUDGET_MINUTES="$1"; shift
  while [ $# -gt 0 ]; do
    case "$1" in
      --budget-hours)   SOAK_BUDGET_MINUTES=$(($2 * 60)); shift 2 ;;
      --budget-minutes) SOAK_BUDGET_MINUTES="$2"; shift 2 ;;
      --jobs|-j)        SOAK_JOBS="$2"; shift 2 ;;
      --numthreads)     SOAK_NUMTHREADS="$2"; shift 2 ;;
      --logdir)         SOAK_LOGDIR="$2"; shift 2 ;;
      # Tests the script, not minion: one round, minimal counts.
      --smoke)          SOAK_SMOKE=1; SOAK_BUDGET_MINUTES=5; shift ;;
      -h|--help)
        sed -n '2,/^$/{ s/^# \{0,1\}//; p; }' "$0"
        echo
        echo "Options:"
        echo "  --budget-hours N     wall-clock budget (default $((SOAK_BUDGET_MINUTES/60))h)"
        echo "  --budget-minutes M   budget in minutes"
        echo "  --jobs N             concurrent tester processes"
        echo "  --numthreads N       threads inside each tester process (default 4)"
        echo "  --logdir DIR         where logs go"
        echo "  --smoke              5-minute self-check that the script works"
        exit 0 ;;
      *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
  done

  # Each tester process runs --numthreads solves concurrently, so the
  # real concurrency is JOBS x NUMTHREADS. Aim that at the core count.
  if [ -z "$SOAK_JOBS" ]; then
    local n; n=$(soak_nproc)
    SOAK_JOBS=$(( n / SOAK_NUMTHREADS ))
    [ "$SOAK_JOBS" -lt 1 ] && SOAK_JOBS=1
  fi

  if [ -z "$SOAK_LOGDIR" ]; then
    SOAK_LOGDIR="$SOAK_REPO/mini-scripts/soak-logs/$(basename "$0" .sh)-$(date +%Y%m%d-%H%M%S)"
  fi
  mkdir -p "$SOAK_LOGDIR"

  SOAK_START=$(date +%s)
  SOAK_DEADLINE=$(( SOAK_START + SOAK_BUDGET_MINUTES * 60 ))
}

soak_remaining() { echo $(( SOAK_DEADLINE - $(date +%s) )); }

soak_budget_spent() { [ "$(soak_remaining)" -le 0 ]; }

# --- builds ----------------------------------------------------------

# Build a minion tree if it isn't already there. Existing builds are
# reused, so re-running a soak after a crash doesn't recompile.
soak_build_minion() {
  local dir="$1"; shift
  if [ -x "$SOAK_REPO/$dir/minion" ]; then
    echo "  reusing $dir/"
    return 0
  fi
  echo "  building $dir/ ($*)"
  mkdir -p "$SOAK_REPO/$dir"
  ( cd "$SOAK_REPO/$dir" \
    && python3 ../configure.py "$@" \
    && make -j"$(soak_nproc)" ) >"$SOAK_LOGDIR/build-$dir.log" 2>&1 \
    || { echo "build of $dir failed, see $SOAK_LOGDIR/build-$dir.log" >&2; exit 1; }
}

soak_build_tester() {
  echo "  building tester"
  ( cd "$SOAK_TESTER_DIR" && cargo build --release ) \
    >"$SOAK_LOGDIR/build-tester.log" 2>&1 \
    || { echo "tester build failed, see $SOAK_LOGDIR/build-tester.log" >&2; exit 1; }
}

# --- constraint list -------------------------------------------------

# Harvest the constraint names from the tester itself rather than
# keeping a copy here. A hardcoded list silently stops covering
# anything added later; tester/big-test-logs/run.sh has 64 names where
# the tester now has 78.
soak_harvest_constraints() {
  local minion="$1" out="$SOAK_LOGDIR/constraints.txt"
  echo "  harvesting constraint list" >&2
  "$SOAK_TESTER" -m "$minion" --count 1 --variant-count 0 \
      --optioncount 0 --ws-max-size 1 \
      >"$SOAK_LOGDIR/harvest.log" 2>&1 || true
  grep -oE '^Tested [^ ]+' "$SOAK_LOGDIR/harvest.log" \
    | awk '{print $2}' | sort -u > "$out"
  local n; n=$(wc -l < "$out" | tr -d ' ')
  if [ "$n" -lt 10 ]; then
    echo "error: harvested only $n constraints from the tester probe." >&2
    echo "  Expected ~78. See $SOAK_LOGDIR/harvest.log" >&2
    exit 1
  fi
  echo "  $n constraints" >&2
  cat "$out"
}

# --- job pool --------------------------------------------------------

# Reads labels on stdin, runs SOAK_JOBS at a time. Each soak defines and
# exports soak_cmd_for to turn a label into a command, so labels stay
# bare words -- names with brackets (check[assign]) would otherwise be
# mangled by xargs item parsing or shell globbing.
#
# Timeouts are clamped to the remaining budget; items reached after the
# budget is gone are recorded SKIPPED, not started.
soak_run_pool() {
  local round="$1" item_timeout="$2"
  export SOAK_ROUND="$round"
  export SOAK_ITEM_TIMEOUT="$item_timeout"
  export SOAK_LOGDIR SOAK_DEADLINE SOAK_TIMEOUT SOAK_STDBUF SOAK_NUMTHREADS
  export SOAK_TESTER SOAK_REPO SOAK_TESTER_DIR
  xargs -P "$SOAK_JOBS" -I{} bash -c 'soak_worker "$@"' _ {}
}

soak_worker() {
  local label="$1"
  local safe; safe=$(printf '%s' "$label" | tr '/[] ' '____')
  local log="$SOAK_LOGDIR/$SOAK_ROUND-$safe.log"
  local rcf="$SOAK_LOGDIR/$SOAK_ROUND-$safe.exit"

  local remaining=$(( SOAK_DEADLINE - $(date +%s) ))
  if [ "$remaining" -le 30 ]; then
    echo "SKIPPED" > "$rcf"
    echo "[$(date +%H:%M:%S)] SKIP (budget) $SOAK_ROUND/$label" >&2
    return 0
  fi
  local t="$SOAK_ITEM_TIMEOUT"
  [ "$t" -gt "$remaining" ] && t="$remaining"

  local cmd; cmd=$(soak_cmd_for "$label")

  local started; started=$(date +%s)
  echo "[$(date +%H:%M:%S)] start $SOAK_ROUND/$label" >&2
  # Line-buffer so a running log is readable while the job is still
  # going -- these run for hours and get checked mid-flight.
  ${SOAK_STDBUF:+$SOAK_STDBUF -oL -eL} \
    "$SOAK_TIMEOUT" --foreground --kill-after=30s "${t}s" \
    bash -c "$cmd" >"$log" 2>&1
  local rc=$?
  echo "$rc" > "$rcf"
  local el=$(( $(date +%s) - started ))
  case "$rc" in
    0)       echo "[$(date +%H:%M:%S)] ok   (${el}s) $SOAK_ROUND/$label" >&2 ;;
    124|137) echo "[$(date +%H:%M:%S)] TIME (${el}s) $SOAK_ROUND/$label" >&2 ;;
    *)       echo "[$(date +%H:%M:%S)] FAIL rc=$rc (${el}s) $SOAK_ROUND/$label" >&2 ;;
  esac
}
export -f soak_worker

# --- summary ---------------------------------------------------------

# A timeout is not a pass -- on these budgets it means the item never
# finished. Reported separately from an outright failure so the two can
# be told apart.
soak_summary() {
  local ok=0 fail=0 tmo=0 skip=0
  {
    echo
    echo "=== $(basename "$0") summary ==="
    echo "budget: ${SOAK_BUDGET_MINUTES}m   used: $(( ($(date +%s) - SOAK_START) / 60 ))m"
    echo "jobs: $SOAK_JOBS x ${SOAK_NUMTHREADS} threads"
    echo
    for f in "$SOAK_LOGDIR"/*.exit; do
      [ -e "$f" ] || continue
      local rc; rc=$(cat "$f")
      local name; name=$(basename "$f" .exit)
      case "$rc" in
        0)        ok=$((ok+1)) ;;
        SKIPPED)  skip=$((skip+1)) ;;
        124|137)  tmo=$((tmo+1)); echo "TIMEOUT: $name" ;;
        *)        fail=$((fail+1)); echo "FAIL rc=$rc: $name" ;;
      esac
    done
    echo
    echo "ok=$ok fail=$fail timeout=$tmo skipped=$skip"
    echo "logs: $SOAK_LOGDIR"
  } | tee "$SOAK_LOGDIR/99-summary.log"

  {
    echo
    echo "=== failure tails ==="
    for f in "$SOAK_LOGDIR"/*.exit; do
      [ -e "$f" ] || continue
      local rc; rc=$(cat "$f")
      [ "$rc" = "0" ] && continue
      [ "$rc" = "SKIPPED" ] && continue
      local name; name=$(basename "$f" .exit)
      echo; echo "--- $name (rc=$rc) ---"
      tail -25 "$SOAK_LOGDIR/$name.log" 2>/dev/null || echo "(no log)"
    done
  } | tee -a "$SOAK_LOGDIR/99-summary.log"

  [ "$fail" -eq 0 ] && [ "$tmo" -eq 0 ]
}
