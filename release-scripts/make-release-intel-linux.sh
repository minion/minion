#!/bin/bash
cd ..
./build-all.sh MYFLAGS=" -march=pentium"
cd release-scripts
./make-release.sh intel-linux
