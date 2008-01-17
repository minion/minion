#!/bin/bash
make veryclean "$@"
make DEBUG=1 "$@" minion  -j2
make minion "$@"  -j2
make generate "$@" -j2
