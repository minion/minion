#!/bin/bash
cd ..
./build-all.sh MYFLAGS=" -fast -arch ppc -mcpu=G4 -mdynamic-no-pic -fomit-frame-pointer"
cd release-scripts
./make-release.sh ppc-mac
