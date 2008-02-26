#!/bin/bash
make veryclean "$@"
make DEBUG=1 "$@" minion  
make minion "$@"  
make generate "$@" 
