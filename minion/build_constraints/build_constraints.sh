#!/bin/sh

echo \#define NO_MAIN > BuildStart.h
echo \#include \"../minion.h\" >> BuildStart.h

echo \#include \"BuildStart.h\"   > BuildStaticStart.cpp
echo \#include \"BuildStart.h\"   > BuildDynamicStart.cpp

echo Constraint\* build_constraint\(ConstraintBlob\& b\) \{ >> BuildStaticStart.cpp
echo switch\(b.constraint.type\) \{ >> BuildStaticStart.cpp

echo DynamicConstraint\* build_dynamic_constraint\(ConstraintBlob\& b\) \{ >> BuildDynamicStart.cpp
echo switch\(b.constraint.type\) \{ >> BuildDynamicStart.cpp

while read f
do
	./build_constraints_2.sh $f
done < ConstraintList


echo \}\} >> BuildStaticStart.cpp
echo \}\} >> BuildDynamicStart.cpp