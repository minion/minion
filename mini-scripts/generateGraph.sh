#!/bin/bash
case $3 in
nauty)
  GAPCOMMAND="generators";;
full)
  GAPCOMMAND="Elements(Group(generators))";;
*)
  echo './generateGraph.sh <minion> <instance> nauty/full'
  exit;;
esac
$1 -Xgraph $2 > $2.gapin
cat `dirname $0`/rule1.g >> $2.gapin
echo 'a := OutputTextFile("'$2.gapout'", false);;' >> $2.gapin
echo 'H :='$GAPCOMMAND';;' >> $2.gapin
echo 'gen_constraints(H, varnames, a);' >> $2.gapin
echo 'quit;' >> $2.gapin
gap.sh < $2.gapin > /dev/null

grep -v "**EOF**" $2 > $2.symbreak
cat $2.gapout >> $2.symbreak
echo '**EOF**' >> $2.symbreak
# cleanup
rm $2.gapin $2.gapout
