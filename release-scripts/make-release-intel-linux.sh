#!/bin/bash
cd ..
./build-all.sh MYFLAGS=" -march=pentium -fomit-frame-pointer"
cd release-scripts
./make-release.sh intel-linux
