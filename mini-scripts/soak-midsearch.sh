#!/bin/bash
# Release soak: the mid-search injection matrix (in-process only).
#
# Sweeps mode x constraint, where mode covers the axes that interact:
# how many constraint packets are injected during one solve, whether
# each injected constraint is wrapped in a random parent, and whether
# fresh variables are added mid-search as well.
#
# The packet-count axis is why this is its own soak. Injecting a single
# constraint exercises far less than injecting two: minion merges the
# per-depth re-propagation sets on the way up, so only with two or more
# constraints do several end up being re-established in the same
# worldPop. A crash that needs N>=2 plus a nested wrapper sat unnoticed
# in exactly that gap.
#
# Everything here needs --in-process, so it runs through minion-sys
# (libminion via FFI), which is also the only way the mid-search entry
# points are reachable at all.
#
# Usage: ./soak-midsearch.sh [--budget-hours N] [--jobs N] [--smoke]

. "$(dirname "$0")/soak-common.sh"
soak_require_tools
soak_parse_args $((36 * 60)) "$@"

echo "=== soak-midsearch: budget ${SOAK_BUDGET_MINUTES}m, ${SOAK_JOBS} jobs x ${SOAK_NUMTHREADS} threads"
soak_build_minion bin-quick --quick
soak_build_tester
export SOAK_MINION="$SOAK_REPO/bin-quick/minion"
CONSTRAINTS=$(soak_harvest_constraints "$SOAK_MINION")

# Each mode is a set of tester flags. Labels are "mode:constraint".
MODES="baseline
addvars
inject1
inject2
inject3
nested1
nested2
nested3
mixed2
mixed3"

soak_mode_flags() {
  case "$1" in
    baseline) echo "" ;;
    addvars)  echo "--midsearch" ;;
    inject1)  echo "--midsearch-constraints" ;;
    inject2)  echo "--midsearch-constraints --midsearch-constraints-num-packets 2" ;;
    inject3)  echo "--midsearch-constraints --midsearch-constraints-num-packets 3" ;;
    nested1)  echo "--midsearch-constraints --midsearch-wrap-nested" ;;
    nested2)  echo "--midsearch-constraints --midsearch-wrap-nested --midsearch-constraints-num-packets 2" ;;
    nested3)  echo "--midsearch-constraints --midsearch-wrap-nested --midsearch-constraints-num-packets 3" ;;
    mixed2)   echo "--midsearch-add-vars --midsearch-constraints-num-packets 2" ;;
    mixed3)   echo "--midsearch-add-vars --midsearch-constraints-num-packets 3" ;;
    *) echo "unknown mode: $1" >&2; return 1 ;;
  esac
}
export -f soak_mode_flags

soak_cmd_for() {
  local mode="${1%%:*}" c="${1#*:}"
  local flags; flags=$(soak_mode_flags "$mode") || return 1
  echo "'$SOAK_TESTER' --in-process --constraints '$c' $flags \
    --count $R_COUNT --variant-count 0 --ws-max-size 1 \
    --size-factor $R_SIZE --var-reuse 0.6 \
    --maxtuples $R_MAXTUPLES --max-solutions 100000 \
    --numthreads $SOAK_NUMTHREADS"
}
export -f soak_cmd_for

#      name    count size maxtuples timeout
ROUNDS="r1-wide   200   1     10000   3600
r2-deeper        500   1     20000   5400
r3-size2         200   2     20000   7200
r4-size4          50   4     20000  10800"

[ "$SOAK_SMOKE" = "1" ] && ROUNDS="smoke 2 1 2000 120"

while read -r name count size maxtuples tmo; do
  [ -z "${name:-}" ] && continue
  if soak_budget_spent; then
    echo "=== budget spent, skipping round $name"
    continue
  fi
  echo "=== round $name (count $count, size-factor $size, $(soak_remaining)s left)"
  export R_COUNT="$count" R_SIZE="$size" R_MAXTUPLES="$maxtuples"
  # mode x constraint
  while IFS= read -r m; do
    [ -z "$m" ] && continue
    while IFS= read -r c; do
      [ -z "$c" ] && continue
      printf '%s:%s\n' "$m" "$c"
    done <<< "$CONSTRAINTS"
  done <<< "$MODES" | soak_run_pool "$name" "$tmo"
done <<< "$ROUNDS"

soak_summary
