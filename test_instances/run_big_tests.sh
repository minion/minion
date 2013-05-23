#!/bin/bash

if [ $# -lt 1 ]; then
  echo Must give a minion binary to test.
  echo Likely values are ../bin/minion or ../bin/minion-debug
  exit 0
fi

if [ ! -x $1 ]
  then
  echo $1 either doesn\'t exist, or isn\'t executable.
  exit 0
fi

exec=$1
#Remove exec from $*, so it only contains parameters
shift
echo Testing $exec with options .$*.


  failed=0

  ./big_do_basic_tests.sh $exec $*
  failed=$(($failed + $?))
  # The following tests take too long!
  ./big_do_random_tests.sh 3 $exec $* -randomiseorder
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 3 $exec $* -varorder random
  failed=$(($failed + $?))
#  ./big_do_random_tests.sh 1 $exec $* -varorder conflict
#  failed=$(($failed + $?))
  ./big_do_random_tests.sh 1 $exec $* -varorder static
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 1 $exec $* -varorder sdf
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 3 $exec $* -varorder sdf-random
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 1 $exec $* -varorder srf
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 3 $exec $* -varorder srf-random
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 1 $exec $* -varorder ldf
  failed=$(($failed + $?))
  ./big_do_random_tests.sh 3 $exec $* -varorder ldf-random
  failed=$(($failed + $?))
  if [ $failed -gt 0 ]; then
    exit $failed
  fi
  
  (cd special_tests; ./special_tests.sh ../$exec)
  failed=$(($failed + $?))
  if [ $failed -gt 0 ]; then
    echo "Some non-critical tests failed."
  fi
