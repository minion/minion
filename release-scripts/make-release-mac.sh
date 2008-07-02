#!/bin/bash
function checkreturn() { if [[ $? != 0 ]]; then echo Fail; exit 1; fi }
cd ..
checkreturn
rm -rf temp-mac-intel-build
checkreturn
mkdir temp-mac-intel-build
checkreturn
rm -rf temp-mac-ppc-build
checkreturn
mkdir temp-mac-ppc-build
checkreturn
./build-all.sh MYFLAGS=" -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fast -arch i386 -march=pentium-m -mdynamic-no-pic -fomit-frame-pointer"
checkreturn
cp bin/minion temp-mac-intel-build/
checkreturn
cp bin/minion-debug temp-mac-intel-build/
checkreturn
# Now build the PPC bits
./build-all.sh MYFLAGS=" -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fast -arch ppc -mcpu=G4 -mdynamic-no-pic -fomit-frame-pointer"
checkreturn
cp bin/minion temp-mac-ppc-build/
checkreturn
cp bin/minion-debug temp-mac-ppc-build/
checkreturn
make veryclean
checkreturn
lipo -create temp-mac-intel-build/minion temp-mac-ppc-build/minion -output bin/minion
checkreturn
lipo -create temp-mac-intel-build/minion-debug temp-mac-ppc-build/minion-debug -output bin/minion-debug
checkreturn
cd release-scripts
checkreturn
./make-release.sh $1-mac
checkreturn
