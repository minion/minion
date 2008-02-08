#!/bin/bash

# First, build constraint_defs.h, which just maps ConstraintList into a valid C struct.
echo > constraint_defs.h
echo ConstraintDef constraint_list[] = \{ >> constraint_defs.h
while read f
do
  echo Building $f
  ./internal_ConstraintDef.sh $f >> constraint_defs.h
done < ConstraintList

echo \{ \"reify\", CT_REIFY, 0, {}, STATIC_CT \}, >> constraint_defs.h
echo \{ \"reifyimply\", CT_REIFYIMPLY, 0, {}, STATIC_CT \}, >> constraint_defs.h
echo \}\; >> constraint_defs.h

# Output a simple header that contains all the constraint identifiers

echo enum ConstraintType \{ > ConstraintEnum.h
awk '{print $3,","}' < ConstraintList >> ConstraintEnum.h
echo CT_REIFY, CT_REIFYIMPLY \}\; >> ConstraintEnum.h

# Now build the machinery which builds all the constraints.

echo > BuildConstraintsStart.h
echo \#define NO_MAIN > BuildStart.h
echo \#include \"../minion.h\" >> BuildStart.h

echo \#include \"BuildStart.h\"   > BuildStaticStart.cpp
echo \#include \"BuildStart.h\"   > BuildDynamicStart.cpp

echo Constraint\* build_constraint\(StateObj* stateObj, ConstraintBlob\& b\) \{ >> BuildStaticStart.cpp
echo switch\(b.constraint.type\) \{ >> BuildStaticStart.cpp

echo DynamicConstraint\* build_dynamic_constraint\(StateObj* stateObj, ConstraintBlob\& b\) \{ >> BuildDynamicStart.cpp
echo switch\(b.constraint.type\) \{ >> BuildDynamicStart.cpp

while read f
do
    echo Building $f
	./internal_buildconstraint.sh $f
done < ConstraintList


echo \}\} >> BuildStaticStart.cpp
echo \}\} >> BuildDynamicStart.cpp
