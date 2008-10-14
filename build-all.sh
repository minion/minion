#!/bin/bash
function checkreturn() { if [[ $? != 0 ]]; then echo Fail; exit 1; fi }
cmake -DDEBUG=1 -DNAME="minion-debug" .
checkreturn
make "$@" minion-debug -j2
checkreturn
cmake -DDEBUG=0 -DNAME="minion" .
checkreturn
make minion "$@" -j2
checkreturn
