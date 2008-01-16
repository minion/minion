#!/bin/sh

	echo BUILD_DEF \#\# $5\($3\) >> BuildConstraintsStart.h
	
	if [ "$1" == "STATIC_CT" ]; then
	  echo case $3 : return build_constraint_$3\(b\)\; >> BuildStaticStart.cpp
	 else
	  echo case $3 : return build_constraint_$3\(b\)\; >> BuildDynamicStart.cpp
	 fi
	
	echo BUILD_DEF_$1\($3\) >> BuildStart.h

    echo \#define NO_MAIN > $3.cpp	
	echo \#include \"../minion.h\" >> $3.cpp
    if [ "$6" = "read_constant_list" ]; then
	  echo BUILD_$1_INITIAL_LIST\($3, $5\) >> $3.cpp
	else
	  echo BUILD_$1\($3, $5\) >> $3.cpp
	fi
	echo >> $3.cpp
	
	