#!/bin/bash
# Release soak: parallel search at sizes where it actually engages.
#
# The work-stealing, portfolio and parallel-preprocess sweeps only test
# anything once instances are big enough that a worker is still busy
# when another goes idle. At the default instance size they finish
# before any donation can happen, so this soak drives --ws-max-size and
# --size-factor up until donations are observed.
#
# That growth is also why this is the one soak whose cost is genuinely
# unbounded: the adaptive sweep doubles instance size per constraint
# until it sees a donation or hits the cap, and that cost does not
# shrink when you lower --count. Per-item timeouts are the only thing
# bounding it -- treat a TIMEOUT here as "this constraint did not finish
# at this size", which is information, not a pass.
#
# Exec backend only: the parallel sweeps spawn minion subprocesses and
# are skipped entirely under --in-process.
#
# Usage: ./soak-parallel.sh [--budget-hours N] [--jobs N] [--smoke]

. "$(dirname "$0")/soak-common.sh"
soak_require_tools
soak_parse_args $((24 * 60)) "$@"

echo "=== soak-parallel: budget ${SOAK_BUDGET_MINUTES}m, ${SOAK_JOBS} jobs x ${SOAK_NUMTHREADS} threads"
soak_build_minion bin-quick --quick
soak_build_tester
export SOAK_MINION="$SOAK_REPO/bin-quick/minion"
CONSTRAINTS=$(soak_harvest_constraints "$SOAK_MINION")

soak_cmd_for() {
  echo "'$SOAK_TESTER' -m '$SOAK_MINION' --constraints '$1' \
    --count $R_COUNT --optioncount $R_OPT --variant-count 0 \
    --size-factor $R_SIZE --ws-max-size $R_WS \
    --var-reuse 0.6 --negate-bool 0.6 \
    --max-solutions 2000000 --numthreads $SOAK_NUMTHREADS"
}
export -f soak_cmd_for

#      name     count opt size  ws  timeout
ROUNDS="r1-ws32    50  100   2   32   5400
r2-ws64            25   50   4   64   7200
r3-ws128           10   25   8  128  10800
r4-ws256            5   10  16  256  14400"

[ "$SOAK_SMOKE" = "1" ] && ROUNDS="smoke 1 1 1 2 120"

while read -r name count opt size ws tmo; do
  [ -z "${name:-}" ] && continue
  if soak_budget_spent; then
    echo "=== budget spent, skipping round $name"
    continue
  fi
  echo "=== round $name (size-factor $size, ws-max-size $ws, $(soak_remaining)s left)"
  export R_COUNT="$count" R_OPT="$opt" R_SIZE="$size" R_WS="$ws"
  printf '%s\n' "$CONSTRAINTS" | soak_run_pool "$name" "$tmo"
done <<< "$ROUNDS"

soak_summary
