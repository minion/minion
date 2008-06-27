#!/bin/bash
./configure.sh
make veryclean "$@"
make DEBUG=1 BOOST=1 "$@" minion NAME="minion-debug" -j2
make minion BOOST=1 "$@"  NAME="minion" -j2
#make generate "$@" 
