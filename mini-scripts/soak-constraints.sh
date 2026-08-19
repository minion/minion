#!/bin/bash
# Release soak: correctness breadth across every constraint.
#
# One tester process per constraint, run in a job pool, over rounds of
# increasing instance size. Each round re-runs the whole constraint set
# with bigger instances and fewer trials, so early rounds go wide and
# later rounds go deep. Rounds stop when the budget runs out.
#
# Process-per-constraint means an abort in one propagator doesn't take
# the rest of the sweep with it -- which matters here, because the
# failures this catches are usually assertion aborts, not clean exits.
#
# --var-reuse and --negate-bool are turned up well above their defaults.
# Those two knobs are what surface aliasing unsoundness (the same
# variable, or a variable and its negation, in two argument positions of
# one constraint), which has been the most productive bug class.
#
# Work-stealing is pinned to --ws-max-size 1 here so rounds stay
# bounded; driving actual donations is soak-parallel.sh's job.
#
# Usage: ./soak-constraints.sh [--budget-hours N] [--jobs N] [--smoke]

. "$(dirname "$0")/soak-common.sh"
soak_require_tools
soak_parse_args $((36 * 60)) "$@"

echo "=== soak-constraints: budget ${SOAK_BUDGET_MINUTES}m, ${SOAK_JOBS} jobs x ${SOAK_NUMTHREADS} threads"
soak_build_minion bin-quick --quick
soak_build_tester
export SOAK_MINION="$SOAK_REPO/bin-quick/minion"
CONSTRAINTS=$(soak_harvest_constraints "$SOAK_MINION")

soak_cmd_for() {
  echo "'$SOAK_TESTER' -m '$SOAK_MINION' --constraints '$1' \
    --count $R_COUNT --optioncount $R_OPT --variant-count $R_VAR \
    --size-factor $R_SIZE --ws-max-size 1 --nest-depth $R_NEST \
    --var-reuse 0.6 --negate-bool 0.6 --numthreads $SOAK_NUMTHREADS"
}
export -f soak_cmd_for

#      name       size count  opt  var nest timeout
ROUNDS="r1-wide      1  2000  2000  500  1   1800
r2-size2            2  1000  1000  200  1   3600
r3-size4            4   500   500  100  2   5400
r4-size8            8   200   200   50  2   7200
r5-size16          16   100   100   25  3  10800"

[ "$SOAK_SMOKE" = "1" ] && ROUNDS="smoke 1 2 2 2 1 60"

while read -r name size count opt var nest tmo; do
  [ -z "${name:-}" ] && continue
  if soak_budget_spent; then
    echo "=== budget spent, skipping round $name"
    continue
  fi
  echo "=== round $name (size-factor $size, count $count, $(soak_remaining)s left)"
  export R_SIZE="$size" R_COUNT="$count" R_OPT="$opt" R_VAR="$var" R_NEST="$nest"
  printf '%s\n' "$CONSTRAINTS" | soak_run_pool "$name" "$tmo"
done <<< "$ROUNDS"

soak_summary
