#!/bin/bash
j=0
for i in *.minion; do
echo -n .
j=$(($j + 1))
if grep "#FAIL" $i > /dev/null;
then
  if $1 -test $2 $i &> /dev/null;
    then echo $i should have failed;
  else
    pass=$(($pass + 1));
  fi
else
  if $1 -test $2 $i &> /dev/null;
    then pass=$(($pass + 1));
  else 
    echo Fail $i;
  fi
fi
done
echo
echo $pass of $j tests successful.
echo Doing randomised tests.
j=0
pass=0
for i in `grep -l "#TEST NOSOLS\|#TEST SOLCOUNT" *.minion`; do
  LOOP=0
  if grep "#FAIL" $i &> /dev/null;
  then
    echo -n 
  else
    while [ $LOOP -lt 10 ]; do
      j=$(($j + 1));
      if $1 -test $2 -randomiseorder $i &> test_output
      then
        pass=$(($pass + 1));
      else
        echo Fail $i. Consult test_output;
        exit;
      fi
      let LOOP=LOOP+1
    done
    fi
  echo -n .
done
echo
echo $pass of $j randomised tests successful.
rm test_output
