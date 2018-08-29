#!/usr/bin/env bash

# Continous integration testing script

# This is currently only used for Travis CI integration, see .travis.yml
# for details. In addition, it can be run manually, to simulate what
# happens in the CI environment locally (say, for debugging purposes).

set -ex

# create dir for coverage results
COVDIR=coverage
mkdir -p $COVDIR




for TEST_SUITE in $TEST_SUITES
do
  case $TEST_SUITE in
  test_instances)
    cd test_instances
    ./run_tests.sh ../bin/minion*
    ;;
  esac
done

exit 0
