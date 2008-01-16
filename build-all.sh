#!/bin/sh
make clean "$@"
make DEBUG=1 "$@" minion
make minion "$@"
make generate "$@"
