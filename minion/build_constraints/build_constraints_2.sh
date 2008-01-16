#!/bin/sh
echo $1 . $2 . $3 . $4 . $5 . $6 . $7 . $8 . $9
	echo BUILD_DEF\($3, $5\) >> BuildConstraintsStart.cpp
	
	rm $3.cpp
	echo \#include \"../minion.h\" > $3.cpp
    if [ "$6" = "read_constant_list" ]; then
	  echo BUILD_$1_INITIAL_LIST\($3, $5\) >> $3.cpp
	else
	  echo BUILD_$1\($3, $5\) >> $3.cpp
	fi
	