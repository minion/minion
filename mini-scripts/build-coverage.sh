#!/bin/bash

if [ "$1" = "" ]; then
  echo Must give HTML output directory
  exit
fi

lcov --directory `dirname $0`/../CMakeFiles/ --capture --output-file temp.info
lcov -r temp.info '/usr/*' > minion.info
rm temp.info
mkdir $1
cd $1
genhtml ../minion.info
rm ../minion.info
