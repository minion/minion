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
stripsols= 
j=0
reify=0
pass=0

for i in *.minion; do
  j=$(($j + 1))
   
  if grep -q "#TEST GROUP" $i;
  then
    # Congratulations, if you understand the next line you are a dodgy shell script master!
    $exec -Xgraph $i $* 2>/dev/null | grep -v "#" > fail_graph
    dreadnaut < fail_graph | ../mini-scripts/dread2gap.py > fail_group
    numsols=`gap.sh < fail_group | tail -n 1 | awk '{print $2}'`
    testnumsols=`grep "#TEST GROUP" $i  | awk '{print $3}' | tr -d '\015' `
    if [[ $(($numsols)) != $(($testnumsols)) ]]; then
      testpass=0
      echo Got ${numsols} instead of ${testnumsols} as group size for $i
      $exec -Xgraph $i $* 2>/dev/null | grep -v "#" > fail_graph
      $exec -Xgraph $i $* 2>/dev/null | grep -v "#" | dreadnaut | ../mini-scripts/dread2gap.py > fail_group
      exit 0
    else
      pass=$(($pass + 1))
    fi
  else
      echo Test $i is not well-formed.
      exit 0
  fi  
   
done

echo
echo $pass of $j tests successful.

exit $(($j - $pass))
