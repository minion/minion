#!/bin/bash
# Release soak: metamorphic optimisation sweep.
#
# Optimisation is the thinnest-covered area in the tester. The ordinary
# sweeps compare solution *sets*, which says nothing about whether a
# reported optimum is right; and the random tester generates no
# MAXIMISING/MINIMISING instances outside this sweep, so all other
# optimisation coverage comes from ~20 hand-written regression files.
#
# The sweep wraps each random instance with `aux = sum(vars)` and a
# random MINIMISING/MAXIMISING, then solves under several (propagator,
# heuristic, parallel) strategies and asserts every one reports the same
# optimum. Disagreement points at bound tracking, cross-worker bound
# broadcast, or restart-with-optimisation interaction -- code that the
# satisfaction sweeps never touch.
#
# Both backends run: exec covers the full strategy set including
# parallel and restarts, in-process covers the Model::optimise FFI
# plumbing with a smaller strategy subset.
#
# Usage: ./soak-optimisation.sh [--budget-hours N] [--jobs N] [--smoke]

. "$(dirname "$0")/soak-common.sh"
soak_require_tools
soak_parse_args $((18 * 60)) "$@"

echo "=== soak-optimisation: budget ${SOAK_BUDGET_MINUTES}m, ${SOAK_JOBS} jobs x ${SOAK_NUMTHREADS} threads"
soak_build_minion bin-quick --quick
soak_build_tester
export SOAK_MINION="$SOAK_REPO/bin-quick/minion"
CONSTRAINTS=$(soak_harvest_constraints "$SOAK_MINION")

# Labels are "backend:constraint".
soak_cmd_for() {
  local backend="${1%%:*}" c="${1#*:}"
  if [ "$backend" = "exec" ]; then
    echo "'$SOAK_TESTER' -m '$SOAK_MINION' --constraints '$c' \
      --optimisation-sweep --count $R_COUNT --size-factor $R_SIZE \
      --var-reuse 0.6 --negate-bool 0.6 --numthreads $SOAK_NUMTHREADS"
  else
    echo "'$SOAK_TESTER' --in-process --constraints '$c' \
      --optimisation-sweep --count $R_COUNT --size-factor $R_SIZE \
      --var-reuse 0.6 --numthreads $SOAK_NUMTHREADS"
  fi
}
export -f soak_cmd_for

#      name    count size timeout
ROUNDS="r1-wide  500    1   3600
r2-size2         200    2   5400
r3-size4          50    4   7200"

[ "$SOAK_SMOKE" = "1" ] && ROUNDS="smoke 2 1 120"

while read -r name count size tmo; do
  [ -z "${name:-}" ] && continue
  if soak_budget_spent; then
    echo "=== budget spent, skipping round $name"
    continue
  fi
  echo "=== round $name (count $count, size-factor $size, $(soak_remaining)s left)"
  export R_COUNT="$count" R_SIZE="$size"
  while IFS= read -r c; do
    [ -z "$c" ] && continue
    printf 'exec:%s\n' "$c"
    printf 'inproc:%s\n' "$c"
  done <<< "$CONSTRAINTS" | soak_run_pool "$name" "$tmo"
done <<< "$ROUNDS"

soak_summary
