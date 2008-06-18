#!/bin/bash


if [ $# -lt 1 ]; then
  echo Must give a minion binary to test.
  echo Likely values are ../bin/minion or ../bin/minion-debug
  exit 0
fi

if [ ! -x $2 ]
then
  echo $1 either doesn\'t exist, or isn\'t executable.
  exit 0
fi

tests=$1
shift
exec=$1
#Remove exec from $*, so it only contains parameters
shift
echo Random testing $exec with options .$*.

j=0
pass=0
for i in `grep -l "#TEST SOLCOUNT" *.minion`; do
  LOOP=0
  if grep "#FAIL\|#BUG" $i &> /dev/null;
  then
    echo -n 
  else
    while [ $LOOP -lt $tests ]; do
      j=$(($j + 1));
      if $exec $* $i &> /dev/null
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
