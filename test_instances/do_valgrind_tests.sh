#!/bin/bash
set -e
set -o errexit
for i in *.minion; do 
  valgrind --error-exitcode=1 --leak-check=full $* -notimers $i -nodelimit 10
done