#!/bin/bash

# Note: Not a proper configure script!

echo == Configuring Minion ==
# First of all rebuild all the files, and get rid of binary files.
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
echo Cleaning up any temporary files.
rm -rf bin/*

echo == Looking for compiler ==

# First check if we even have a working compiler...
if g++ build_tests/tiny_test.cpp -o build_test_temp &>/dev/null; then
  echo Compiler found
else
  echo g++ is Missing or the Minion source is completely broken. Giving up!
  { (exit 1); exit 1; };
fi

rm -f Makefile.includes > /dev/null
echo \# BOOST=1 > Makefile.includes
echo \# BOOSTINCLUDE=-I/usr/local/include/boost-1_35/ >> Makefile.includes

echo == Looking for optional components ==
if g++ build_tests/tiny_boost_test.cpp -I/usr/local/include/boost-1_35/ -o build_test_temp &>/dev/null; then
  echo Boost found in proper place.
  echo BOOST=1 > Makefile.includes
  echo BOOSTINCLUDE=-I/usr/local/include/boost-1_35/ >> Makefile.includes
else
  echo Warning : Cannot found boost. This is only an optional com
  echo You will not be able to use compressed input files.
  echo You can edit Makefile.includes to set your Boost location.
fi


if g++ build_tests/tr1_test.cpp -o build_test_temp &>/dev/null; then
  echo Found TR1 - Using hashed containers
  echo MYFLAGS := -DUSE_TR1_HASH_MAP_AND_SET > Makefile.includes
else
  echo No TR1 - Using tree containers
fi

echo \# This next line is just to allow the Makefile to check this file exists, >> Makefile.includes
echo \# and is vaguely sane. >> Makefile.includes
echo SETUP_INCLUDED=1 >> Makefile.includes


# Clean up
rm -f build_test_temp_1 > /dev/null