#!/bin/bash
make veryclean "$@"
make DEBUG=1 BOOST=1 "$@" minion  
make minion BOOST=1 "$@"  
make generate "$@" 
