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

pass=0

echo Doing randomised tests.
j=0
pass=0
for i in `grep -l "#TEST SOLCOUNT" *.minion`; do
  LOOP=0
  if grep "#FAIL\|#BUG" $i &> /dev/null;
  then
    echo -n 
  else
    while [ $LOOP -lt 10 ]; do
      j=$(($j + 1));
      if $1 $2 -randomiseorder $i &> /dev/null
      then
        pass=$(($pass + 1));
      else
        echo Fail $i;
      fi
      let LOOP=LOOP+1
    done
    fi
  echo -n .
done
echo
echo $pass of $j randomised tests successful.

