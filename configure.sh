#!/bin/bash

# Note: Not a proper configure script!

echo Build svn header.
rm -f minion/svn_header.h &> /dev/null
mini-scripts/get_svn_version.sh minion/svn_header.h
echo Building Dependancy list.
rm -f Makefile.dep Makefile.dep.temp &> /dev/null
mini-scripts/make_depend.sh
echo Rebuilding Constraint list.
cd minion/build_constraints
./rebuild_files.sh
cd ../..
echo Emptying 'bin' directory
rm -rf bin/*
if g++ build_tests/tiny_test.cpp -o build_test_temp; then
  echo Compiler found
else
  echo g++ is Missing! Fatal error.
fi

rm -f Makefile.includes > /dev/null
echo \# BOOST=1 > Makefile.includes
echo \# BOOSTINCLUDE=-I/usr/local/include/boost-1_35/ >> Makefile.includes

if g++ build_tests/tiny_boost_test.cpp -I/usr/local/include/boost-1_35/ -o build_test_temp; then
  echo Boost found in proper place.
  echo BOOST=1 > Makefile.includes
  echo BOOSTINCLUDE=-I/usr/local/include/boost-1_35/ >> Makefile.includes
else
  echo Boost is not at: /usr/local/include/boost-1_35/ 
  echo You will not be able to use compressed input files.
  echo You can edit Makefile.includes to set your Boost location.
fi

echo \# This next line is just to allow the Makefile to check this file exists, >> Makefile.includes
echo \# and is vaguely sane. >> Makefile.includes
echo SETUP_INCLUDED=1 >> Makefile.includes

# Clean up
rm -f build_test_temp_1 > /dev/null