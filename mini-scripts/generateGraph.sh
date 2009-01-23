#!/bin/bash

grep "varnames :=" $1 | cut -c2- > $1.originalnames
grep -v "^#" $1 > $1.dreadnautin
dreadnaut < $1.dreadnautin | mini-scripts/dread2gap.py > $1.gapin
cat $1.originalnames >> $1.gapin
cat mini-scripts/rule1.g >> $1.gapin
echo 'a := OutputTextFile("'$1.gapout'", false);;' >> $1.gapin
echo 'gen_constraints(G, varnames, a);' >> $1.gapin
echo 'quit;' >> $1.gapin
#gap.sh < $1.gapin