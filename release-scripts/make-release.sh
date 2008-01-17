#!/bin/sh
#./build-all.sh MYFLAGS="-arch ppc -arch i386"
cd ..
rm -rf minion-release-$1
mkdir minion-release-$1
cp bin/* minion-release-$1
strip minion-release-$1/*
cp docs/manual.pdf minion-release-$1
cp LICENSE.txt minion-release-$1
cp -r test_instances minion-release-$1
cp -r benchmarks minion-release-$1
cp Changelog minion-release-$1
find ./minion-release-$1 -type d -name '.svn' -print0 | xargs -0 rm -rdf
rm minion-$1.zip
zip -r -9 minion-$1.zip minion-release-$1