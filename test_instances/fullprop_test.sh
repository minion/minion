for i in *.minion; do
  test1=`../bin/minion-debug $i | ../mini-scripts/total_nodes.sh` 
  test2=`../bin/minion-debug -fullprop $i | ../mini-scripts/total_nodes.sh`
  if [ "X$test1" != "X$test2" ]; then
    echo $test1 $test2 $i
  fi
done