#!/bin/bash
cd ..
rm -rf temp-mac-intel-build
mkdir temp-mac-intel-build
rm -rf temp-mac-ppc-build
mkdir temp-mac-ppc-build
./build-all.sh MYFLAGS=" -fast -arch i386 -march=pentium-m -mdynamic-no-pic -fomit-frame-pointer"
cp bin/* temp-mac-intel-build/
# Now build the PPC bits
./build-all.sh MYFLAGS=" -fast -arch ppc -mcpu=G4 -mdynamic-no-pic -fomit-frame-pointer"
cp bin/* temp-mac-ppc-build/
make veryclean
lipo -create temp-mac-intel-build/minion temp-mac-ppc-build/minion -output bin/minion
lipo -create temp-mac-intel-build/minion-debug temp-mac-ppc-build/minion-debug -output bin/minion-debug
lipo -create temp-mac-intel-build/bibd temp-mac-ppc-build/bibd -output bin/bibd
lipo -create temp-mac-intel-build/golomb temp-mac-ppc-build/golomb -output bin/golomb
lipo -create temp-mac-intel-build/solitaire temp-mac-ppc-build/solitaire -output bin/solitaire
lipo -create temp-mac-intel-build/steelmill temp-mac-ppc-build/steelmill -output bin/steelmill
lipo -create temp-mac-intel-build/sports temp-mac-ppc-build/sports -output bin/sports

cd release-scripts
./make-release.sh mac
