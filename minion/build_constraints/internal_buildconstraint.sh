#!/bin/sh

  count=0
  for i in $*
  do
    echo $i
    case "$i" in
      "read_list")
        count=$((count + 1))
      ;;
      "read_var")
        count=$((count + 1))
      ;;
      "read_2_vars")
        count=$((count + 1))
      ;;
    esac
  done
  
  
	echo BUILD_DEF \#\# $count\($3\) >> BuildConstraintsStart.h
	
	if [ "$1" = "STATIC_CT" ]; then
	  echo case $3 : return build_constraint_$3\(stateObj, b\)\; >> BuildStaticStart.cpp
	 else
	  echo case $3 : return build_constraint_$3\(stateObj, b\)\; >> BuildDynamicStart.cpp
	 fi
	
	echo BUILD_DEF_$1\($3\) >> BuildStart.h

# This might look like a mess, but it's just to get the formatting how it was
# done manually, to avoid nasty svn diffs for no good reason.

	echo \#include \"../minion.h\" > $3.cpp
    echo \/\* Minion Constraint Solver >> $3.cpp
    echo \ \  http://minion.sourceforge.net >> $3.cpp
    echo \ \ \ >> $3.cpp
    echo \ \   For Licence Information see file LICENSE.txt\ >> $3.cpp
    echo >> $3.cpp
    echo \ \   \$Id\$ >> $3.cpp
    echo */ >> $3.cpp
    echo >> $3.cpp
	
	for include in `./only_filename_grep.sh $3 ../constraints/* ../dynamic_constraints/* ../dynamic_constraints/unary/*`; do
	echo \#include \"$include\" >> $3.cpp;
	done
	
	
 	echo BUILD_$1\($3, $count\) >> $3.cpp
	echo >> $3.cpp
	
	