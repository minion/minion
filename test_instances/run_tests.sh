#!/bin/bash
j=0
reify=0
pass=0
expectedfail=0
for i in *.minion; do
echo -n .
j=$(($j + 1))
if grep "#FAIL" $i > /dev/null;
then
  if $1 -test $2 $i &> /dev/null;
    then echo "$i should have failed";
  else
    expectedfail=$(($expectedfail + 1));
    echo "Expected failure occurred - $i"
  fi
else
  if $1 -test $2 $i &> /dev/null;
    then pass=$(($pass + 1));
  else 
    $1 -test $2 $i &> test_output;
    if grep -q "Reification is not" test_output;
    then
      echo "Reification failure - $i";
      reify=$(($reify + 1));
    else    
      echo "Fail $i";
    fi
  fi
fi
done
echo
echo "$pass of $j tests successful."
echo $(($j - $pass - $reify - $expectedfail)) tests failed due to unexpected errors.
echo $reify tests failed due to unimplemented reification.
if [[ ! ($expectedfail -eq 0) ]]
then 
   echo $expectedfail tests failed due to other expected errors,  e.g. known bugs
fi

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
