#!/bin/bash
#
# Run only the do_random_tests.sh invocations from run_tests.sh.
#
# These tests only check the *count* of solutions (not specific solutions
# or node counts), so they are safe to run under variable search orderings
# — including parallel modes that change exploration order
# (-X-parallelWorkSteal, -X-parallelThreads at N=1, -parallel, etc).
#
# do_basic_tests.sh's CHECKONESOL / NODECOUNT tests are sensitive to
# search order and therefore not safe under those modes; this is the
# subset that *is* safe.
#
# Usage:  ./run_random_tests.sh <minion-binary> [extra flags...]
# Example: ./run_random_tests.sh ../bin-ws/minion -X-parallelWorkSteal 4

if [ $# -lt 1 ]; then
  echo "Must give a minion binary to test."
  echo "Likely values are ../bin/minion or ../bin/minion-debug"
  exit 1
fi

if [ ! -x $1 ]; then
  echo "$1 either doesn't exist, or isn't executable."
  exit 1
fi

exec=$1
shift
echo "Random testing $exec with options .$*."

failed=0

./do_random_tests.sh 3 $exec $* -randomiseorder
failed=$(($failed + $?))
./do_random_tests.sh 3 $exec $* -varorder random
failed=$(($failed + $?))
./do_random_tests.sh 1 $exec $* -varorder static
failed=$(($failed + $?))
./do_random_tests.sh 1 $exec $* -varorder sdf
failed=$(($failed + $?))
./do_random_tests.sh 3 $exec $* -varorder sdf-random
failed=$(($failed + $?))
./do_random_tests.sh 1 $exec $* -varorder srf
failed=$(($failed + $?))
./do_random_tests.sh 3 $exec $* -varorder srf-random
failed=$(($failed + $?))
./do_random_tests.sh 1 $exec $* -varorder ldf
failed=$(($failed + $?))
./do_random_tests.sh 3 $exec $* -varorder ldf-random
failed=$(($failed + $?))

if $exec | grep "wdeg on" > /dev/null; then
  ./do_random_tests.sh 3 $exec $* -varorder wdeg
  failed=$(($failed + $?))
  ./do_random_tests.sh 3 $exec $* -varorder domoverwdeg
  failed=$(($failed + $?))
fi

if [ $failed -gt 0 ]; then
  echo "$failed random tests failed."
  exit $failed
fi

echo "All random tests passed."
exit 0
