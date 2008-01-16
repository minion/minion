#!/bin/sh
while read f
do
	./build_constraints_2.sh $f
done < ConstraintList