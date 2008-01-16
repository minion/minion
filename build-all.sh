#!/bin/bash
make veryclean "$@"
make DEBUG=1 "$@" minion -j 2
make minion "$@" -j 2
make generate "$@" -j 2
