#!/bin/bash
set -e
set -o errexit

if which valgrind; then
  for i in *.minion; do
    if grep '#FAIL' $i > /dev/null; then
      :
    elif grep '#TEST EXITCODE1' $i > /dev/null; then
      :
    else
      valgrind --error-exitcode=1 --leak-check=full $* $i -nodelimit 10
    fi
  done
else
  echo 'no valgrind, skipping valgrind tests'
fi
