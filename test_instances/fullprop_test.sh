for i in *.minion; do
  test1=`../bin/minion-debug $i | ../mini-scripts/get_info.sh nodes` 
  test2=`../bin/minion-debug -fullprop $i | ../mini-scripts/get_info.sh nodes`
  if [ "X$test1" != "X$test2" ]; then
    echo $test1 $test2 $i
  fi
done
