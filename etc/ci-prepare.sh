#!/usr/bin/env bash

set -ex

# for debugging purposes print the environment and info about the HEAD commit
printenv | sort
git show --pretty=fuller -s

# prepare the build system
mkdir bin
cd bin
../configure.py $CONFIGFLAGS
make -j4

