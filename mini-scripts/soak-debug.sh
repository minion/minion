#!/bin/bash
# Release soak: the same ground again, with assertions on.
#
# Assertions are where this codebase's bugs surface. A stale propagator
# structure usually still yields an answer under -O3; under D_ASSERT it
# aborts at the mistake instead of returning a wrong count elsewhere.
#
# Three layers, each reaching something the others do not:
#
#   exec    bin-debug: D_ASSERT, checked DomainInt, _GLIBCXX_DEBUG.
#           Stock bin-quick has none of these.
#   inproc  in-process with minion-sys's dom-assert feature -- D_ASSERT
#           on top of an optimised build. minion-sys's default is
#           optimised with assertions off, so the feature is what makes
#           this layer worth running.
#   dbgsys  in-process with DEBUG_MINION=1, adding the checked-integer
#           and libstdc++ debug layers to the FFI path.
#
# inproc and dbgsys each use their own cargo target dir: changing a
# minion-sys feature or DEBUG_MINION on a shared one rebuilds libminion
# every time it changes.
#
# Counts are lower than the other soaks because all of this is slower.
# Breadth over depth -- many shapes through the assertions beats one
# shape for a long time.
#
# Usage: ./soak-debug.sh [--budget-hours N] [--jobs N] [--smoke]

. "$(dirname "$0")/soak-common.sh"
soak_require_tools
soak_parse_args $((24 * 60)) "$@"

echo "=== soak-debug: budget ${SOAK_BUDGET_MINUTES}m, ${SOAK_JOBS} jobs x ${SOAK_NUMTHREADS} threads"
soak_build_minion bin-quick --quick
soak_build_minion bin-debug --quick --debug
soak_build_tester
export SOAK_MINION="$SOAK_REPO/bin-quick/minion"
export SOAK_MINION_DEBUG="$SOAK_REPO/bin-debug/minion"

# Assertions on an otherwise optimised libminion. This is the layer that
# catches a stale propagator structure at the mistake rather than as a wrong
# count somewhere later, without paying for a full debug build.
export SOAK_ASSERT_TARGET="$SOAK_TESTER_DIR/target-domassert"
export SOAK_TESTER_ASSERT="$SOAK_ASSERT_TARGET/release/tester"
echo "  building tester with minion-sys/dom-assert"
( cd "$SOAK_TESTER_DIR" && CARGO_TARGET_DIR="$SOAK_ASSERT_TARGET" \
    cargo build --release --features minion-sys/dom-assert ) \
  >"$SOAK_LOGDIR/build-tester-domassert.log" 2>&1 \
  || { echo "dom-assert tester build failed, see $SOAK_LOGDIR/build-tester-domassert.log" >&2; exit 1; }

# Separate target dir so toggling DEBUG_MINION doesn't invalidate the
# normal tester build on every round.
export SOAK_DBG_TARGET="$SOAK_TESTER_DIR/target-debugminion"
export SOAK_TESTER_DBG="$SOAK_DBG_TARGET/release/tester"
echo "  building tester with DEBUG_MINION=1"
( cd "$SOAK_TESTER_DIR" && DEBUG_MINION=1 CARGO_TARGET_DIR="$SOAK_DBG_TARGET" \
    cargo build --release ) >"$SOAK_LOGDIR/build-tester-debugminion.log" 2>&1 \
  || { echo "DEBUG_MINION tester build failed, see $SOAK_LOGDIR/build-tester-debugminion.log" >&2; exit 1; }

CONSTRAINTS=$(soak_harvest_constraints "$SOAK_MINION")

# The regression suite under the debug binary is cheap and catches
# things the random tester's instance shapes never generate.
echo "=== .minion regression suite (bin-debug)"
( cd "$SOAK_REPO/test_instances" && bash ./run_tests.sh "$SOAK_MINION_DEBUG" ) \
  >"$SOAK_LOGDIR/regression-debug.log" 2>&1
echo "$?" > "$SOAK_LOGDIR/regression-debug.exit"

# Labels are "layer:constraint".
soak_cmd_for() {
  local layer="${1%%:*}" c="${1#*:}"
  case "$layer" in
    exec)
      echo "'$SOAK_TESTER' -m '$SOAK_MINION_DEBUG' --constraints '$c' \
        --count $R_COUNT --optioncount $R_OPT --variant-count $R_VAR \
        --size-factor $R_SIZE --ws-max-size 1 --nest-depth 2 \
        --var-reuse 0.6 --negate-bool 0.6 --numthreads $SOAK_NUMTHREADS" ;;
    inproc)
      echo "'$SOAK_TESTER_ASSERT' --in-process --constraints '$c' \
        --midsearch-constraints --midsearch-wrap-nested \
        --midsearch-constraints-num-packets 2 \
        --count $R_COUNT --variant-count 0 --ws-max-size 1 \
        --size-factor $R_SIZE --var-reuse 0.6 --maxtuples 10000 \
        --max-solutions 100000 --numthreads $SOAK_NUMTHREADS" ;;
    dbgsys)
      echo "'$SOAK_TESTER_DBG' --in-process --constraints '$c' \
        --count $R_COUNT --variant-count 0 --ws-max-size 1 \
        --size-factor $R_SIZE --var-reuse 0.6 --maxtuples 10000 \
        --max-solutions 100000 --numthreads $SOAK_NUMTHREADS" ;;
    *) echo "unknown layer: $layer" >&2; return 1 ;;
  esac
}
export -f soak_cmd_for

#      name    count opt var size timeout
ROUNDS="r1-wide   200 200 100   1   5400
r2-size2          100 100  50   2   7200
r3-size4           25  25  10   4  10800"

[ "$SOAK_SMOKE" = "1" ] && ROUNDS="smoke 2 2 2 1 180"

while read -r name count opt var size tmo; do
  [ -z "${name:-}" ] && continue
  if soak_budget_spent; then
    echo "=== budget spent, skipping round $name"
    continue
  fi
  echo "=== round $name (count $count, size-factor $size, $(soak_remaining)s left)"
  export R_COUNT="$count" R_OPT="$opt" R_VAR="$var" R_SIZE="$size"
  while IFS= read -r c; do
    [ -z "$c" ] && continue
    printf 'exec:%s\n'   "$c"
    printf 'inproc:%s\n' "$c"
    printf 'dbgsys:%s\n' "$c"
  done <<< "$CONSTRAINTS" | soak_run_pool "$name" "$tmo"
done <<< "$ROUNDS"

soak_summary
