#!/bin/sh
#./build-all.sh MYFLAGS="-arch ppc -arch i386"
rm -rf minion-release
mkdir minion-release
cp bin/* minion-release
strip minion-release/*
cp docs/manual.pdf minion-release
cp LICENSE.txt minion-release
cp -r test_instances minion-release
cp -r benchmarks minion-release
find ./minion-release -type d -name '.svn' -print0 | xargs -0 rm -rdf
rm minion.zip
zip -r -9 minion.zip minion-release