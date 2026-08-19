#!/bin/bash
# Run the whole release soak suite, one soak at a time.
#
# The soaks run sequentially, not concurrently: each one already fills
# the machine with a job pool sized to the core count, so overlapping
# them would just make every measurement contend with every other.
#
# Default budgets total 138 hours (~5.75 days), each soak under the
# two-day mark:
#
#   soak-constraints    36h   breadth across every constraint
#   soak-midsearch      36h   mid-search injection matrix
#   soak-parallel       24h   work-stealing at sizes that engage it
#   soak-optimisation   18h   metamorphic optimum agreement
#   soak-debug          24h   all of the above with assertions live
#
# --budget-hours caps the TOTAL. Each soak gets the lesser of its own
# default and whatever is left, so a short total still runs every soak
# rather than spending everything on the first one.
#
# A failing soak does not stop the rest -- the whole point of running
# these before a release is to find out everything that is wrong, not
# the first thing. The final exit status is non-zero if any soak failed.
#
# Usage: ./run-all-soaks.sh [--budget-hours N] [--jobs N] [--smoke]

set -u
set -o pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
TOTAL_MINUTES=$((138 * 60))
PASS_THROUGH=()
SMOKE=0

while [ $# -gt 0 ]; do
  case "$1" in
    --budget-hours)   TOTAL_MINUTES=$(($2 * 60)); shift 2 ;;
    --budget-minutes) TOTAL_MINUTES="$2"; shift 2 ;;
    --smoke)          SMOKE=1; PASS_THROUGH+=("--smoke"); shift ;;
    --jobs|-j|--numthreads) PASS_THROUGH+=("$1" "$2"); shift 2 ;;
    -h|--help)
      sed -n '2,/^$/{ s/^# \{0,1\}//; p; }' "$0"; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
done

#      script                default-minutes
SOAKS="soak-constraints.sh    2160
soak-midsearch.sh             2160
soak-parallel.sh              1440
soak-optimisation.sh          1080
soak-debug.sh                 1440"

TS=$(date +%Y%m%d-%H%M%S)
RUNDIR="$HERE/soak-logs/all-$TS"
mkdir -p "$RUNDIR"

START=$(date +%s)
DEADLINE=$(( START + TOTAL_MINUTES * 60 ))
FAILED=()

while read -r script mins; do
  [ -z "${script:-}" ] && continue
  remaining=$(( (DEADLINE - $(date +%s)) / 60 ))
  if [ "$remaining" -lt 2 ]; then
    echo "=== total budget spent, not starting $script"
    FAILED+=("$script (not started: no budget)")
    continue
  fi
  budget="$mins"
  [ "$budget" -gt "$remaining" ] && budget="$remaining"

  echo
  echo "############################################################"
  echo "### $script  (budget ${budget}m, ${remaining}m left overall)"
  echo "############################################################"
  if [ "$SMOKE" = "1" ]; then
    "$HERE/$script" ${PASS_THROUGH[@]+"${PASS_THROUGH[@]}"} \
      --logdir "$RUNDIR/${script%.sh}"
  else
    "$HERE/$script" --budget-minutes "$budget" \
      ${PASS_THROUGH[@]+"${PASS_THROUGH[@]}"} \
      --logdir "$RUNDIR/${script%.sh}"
  fi
  rc=$?
  [ "$rc" -ne 0 ] && FAILED+=("$script (rc=$rc)")
done <<< "$SOAKS"

echo
echo "############################################################"
echo "### release soak suite: $(( ($(date +%s) - START) / 60 ))m total"
echo "### logs: $RUNDIR"
if [ "${#FAILED[@]}" -eq 0 ]; then
  echo "### all soaks passed"
  exit 0
fi
echo "### FAILURES:"
printf '###   %s\n' "${FAILED[@]}"
exit 1
