#!/bin/sh
python benchmarks/scripts/benchmark-hudson.py $1 bibd benchmarks/Bibd/bibdline*[0-9].minion
python benchmarks/scripts/benchmark-hudson.py $1 bibd-watched benchmarks/Bibd/bibdline*watched.minion
python benchmarks/scripts/benchmark-hudson.py $1 solitaire benchmarks/solitaire/*.minion
python benchmarks/scripts/benchmark-hudson.py $1 SAT benchmarks/SAT/*.minion
python benchmarks/scripts/benchmark-hudson.py $1 quasigroup-element benchmarks/Quasigroup/qg-element*.minion
python benchmarks/scripts/benchmark-hudson.py $1 quasigroup-gacelement benchmarks/Quasigroup/qg-gacelement*.minion
python benchmarks/scripts/benchmark-hudson.py $1 quasigroup-watchelement benchmarks/Quasigroup/qg-watchelement*.minion
python benchmarks/scripts/benchmark-hudson.py $1 graceful-simple benchmarks/graceful/k[3456]p2_*simple.minion
python benchmarks/scripts/benchmark-hudson.py $1 graceful-table benchmarks/graceful/k[3456]p2_*table.minion
