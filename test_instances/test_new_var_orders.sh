#!/bin/bash

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
