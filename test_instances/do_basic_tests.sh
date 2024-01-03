#!/bin/bash

j=0
reify=0
pass=0
expectedfail=0
unexpectedpass=0


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

for i in *.minion; do
  printf "."
  j=$(($j + 1))

  # This gives the value that the program should return.
  if grep -q "#FAIL" $i;
    then 
    correctoutput=0
    else
    correctoutput=1  
  fi

  if grep -q "#BUG" $i;
    then 
    bug=1
    else 
    bug=0
  fi

  extraflags=`grep "#TEST EXTRAFLAGS" $i | awk '{print $3}'`
  if grep -q "#TEST SOLCOUNT" $i;
    then
    numsols=`$exec $i $extraflags -findallsols $* 2>/dev/null | ../mini-scripts/get_info.sh solutions`
    testnumsols=`grep "#TEST SOLCOUNT" $i  | awk '{print $3}' | tr -d '\015' `
    if [[ "$numsols" != "$testnumsols" ]]; then
      testpass=0
      errormess="Got '${numsols}' instead of '${testnumsols}' solutions in $i"
    else
      testpass=1
    fi
  else
    if grep -q "#TEST CHECKONESOL" $i; then
      sol=`$exec $i $* $extraflags 2>/dev/null | ../mini-scripts/print_sol.sh`
      # That "tr" is just to deal with line ending problems.
      testsol=`grep "#TEST CHECKONESOL" $i | awk '{$1 = ""; $2 = ""; print }' | tr -d '\015' `

      # This horrible mess just strips the given solutions into a comparable
      # format. It turns " 1   2   3   4 " into "1,2,3,4". 
      sol=`echo $sol | sed -e "s/^ \{1,\}//;s/ \{1,\}$//;s/ \{1,\}/,/g;"`
      testsol=`echo $testsol | sed -e "s/^ \{1,\}//;s/ \{1,\}$//;s/ \{1,\}/,/g;"`
      if [[ "$sol" = "$testsol" ]]; then
        testpass=1
        else
        testpass=0
        errormess="Got '${sol}' instead of '${testsol}' as solution in $i"
      fi
    else
      if grep -q "#TEST NODECOUNT" $i;
        then
        numnodes=`$exec $i $* $extraflags 2>/dev/null | ../mini-scripts/get_info.sh nodes`
        testnumnodes=`grep "#TEST NODECOUNT" $i  | awk '{print $3}' | tr -d '\015' `
        if [[ "$numnodes" != "$testnumnodes" ]]; then
          testpass=0
          errormess="Got '${numnodes}' instead of '${testnumnodes}' search nodes in $i"
        else
          testpass=1
        fi
      elif grep -q '#TEST EXITCODE1' $i
        then
        return_code=`$exec $i $* $extraflags 2>/dev/null >/dev/null; echo $?`
        if [[ $return_code -eq  1 ]]; then
          testpass=1
        else
          testpass=0
          errormess="Got return code of $return_code, expected 1 in $i"
        fi
      else
        echo Test $i is not well-formed.
        exit 1
      fi  
    fi
  fi

    if [ "$bug" = "0" ]; then
      if [ "$testpass" = "$correctoutput" ]; then
        pass=$(($pass + 1))
        else
        if [ "$correctoutput" = "0" ]; then
          echo Expected $i to fail.
          else
          echo $errormess
        fi
      fi
      else
      if [ "$testpass" = "$correctoutput" ]; then
        echo $i passed, but is supposed to be buggy!
        unexpectedpass=$(($unexpectedpass + 1))
        else
        expectedfail=$(($expectedfail + 1))
      fi
    fi  
  done

  echo
  echo $pass of $j tests successful.
  echo $(($j - $pass - $expectedfail)) tests failed due to unexpected errors.
  echo $expectedfail tests failed due to expected errors.
  echo $unexpectedpass tests passed unexpectedly.

exit $(($j - $pass - $expectedfail))
