#!/bin/bash
cd ..
./build-all.sh MYFLAGS=" -fast -march=pentium-m -mdynamic-no-pic -fomit-frame-pointer"
cd release-scripts
./make-release.sh intel-mac
