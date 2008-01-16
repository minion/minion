#!/bin/bash
cd ..
./build-all.sh MYFLAGS=" -fast -march=pentium-m -mdynamic-no-pic "
cd release-scripts
./make-release.sh intel-mac
