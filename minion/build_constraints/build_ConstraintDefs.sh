#!/bin/sh
echo > constraint_defs.h
echo ConstraintDef constraint_list[] = \{ >> constraint_defs.h
while read f
do
  echo Building $f
  ./build_ConstraintDefs_2.sh $f >> constraint_defs.h
done < ConstraintList

echo \{ \"reify\", CT_REIFY, 0, {}, STATIC_CT \}, >> constraint_defs.h
echo \{ \"reifyimply\", CT_REIFYIMPLY, 0, {}, STATIC_CT \}, >> constraint_defs.h
echo \}\; >> constraint_defs.h