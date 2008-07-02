#!/bin/bash
function checkreturn() { if [[ $? != 0 ]]; then echo Fail; exit 1; fi }
./configure.sh
checkreturn
make veryclean "$@"
checkreturn
make DEBUG=1 "$@" minion NAME="minion-debug" -j2
checkreturn
make minion "$@"  NAME="minion" -j2
checkreturn
