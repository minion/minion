#!/bin/bash
cd ..
./build-all.sh MYFLAGS=" -fast -arch ppc -march=G4 -mdynamic-no-pic "
cd release-scripts
./make-release.sh ppc-mac
