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

  ./do_basic_tests.sh $exec $*
  failed=$(($failed + $?))
  # The following tests take too long!
  ./do_random_tests.sh 3 $exec $* -randomiseorder
  failed=$(($failed + $?))
  ./do_random_tests.sh 3 $exec $* -varorder random
  failed=$(($failed + $?))
#  ./do_random_tests.sh 1 $exec $* -varorder conflict
#  failed=$(($failed + $?))
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
  
  (cd special_tests; ./special_tests.sh ../$exec)
  failed=$(($failed + $?))
  
  exit $failed

