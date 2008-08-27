#!/bin/bash
function checkreturn() { if [[ $? != 0 ]]; then echo Fail; exit 1; fi }
cd ..
checkreturn
rm -rf minion-release-$1
checkreturn
rm -rf minion-$1.zip
checkreturn
mkdir minion-release-$1
checkreturn
cp bin/minion minion-release-$1
checkreturn
cp bin/minion-debug minion-release-$1
checkreturn
strip minion-release-$1/*
checkreturn
# This is special, we don't want to check if this fails as it is only
# for cygwin. Also it must not be stripped.
cp /usr/bin/cygwin1.dll minion-release-$1
cp -r docs/htmlhelp minion-release-$1
checkreturn
cp manual/Manual.pdf minion-release-$1
checkreturn
cp -r summer_school minion-release-$1 
checkreturn
cp LICENSE.txt minion-release-$1
checkreturn
cp -r test_instances minion-release-$1
checkreturn
cp -r benchmarks minion-release-$1
checkreturn
cp Changelog minion-release-$1
checkreturn
cp translator/tailor.jar minion-release-$1/tailor.jar
checkreturn
cp translator/README.release minion-release-$1/README.tailor
checkreturn
cp -r translator/examples minion-release-$1/tailor-examples
checkreturn
cp -r translator/syntax-highlighting minion-release-$1/essence-syntax-highlighting
checkreturn
cp -r translator/doc minion-release-$1/tailor-doc
checkreturn
find ./minion-release-$1 -type d -name '.svn' -print0 | xargs -0 rm -rdf
checkreturn
zip -r -9 minion-$1.zip minion-release-$1
checkreturn
