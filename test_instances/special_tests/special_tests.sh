#!/bin/bash


solscript=../../mini-scripts/solutions.sh

if [ $# -lt 1 ]; then
  echo Must give a minion binary to test.
  echo Likely values are ../bin/minion or ../bin/minion-debug
  exit 0
fi

if [ ! -x $1 ]
  then
  echo $1 either doesn\'t exist, or isn\'t executable.
  exit 0
fi

exec=$1
#Remove exec from $*, so it only contains parameters
shift

if [[ "`$exec test_bzip2.minion.bz2 | $solscript`" != "1" ]]; then
  echo Bzip2 test failed
  exit 1
fi

if [[ "`$exec test_gzip.minion.gz | $solscript`" != "1" ]]; then
  echo gzip test failed
  exit 1
fi

if [[ "`$exec bibd.minion.bz2 -preprocess SAC | grep ^SAC | print 3`" != "13" ]]; then
  echo SAC test failed
  exit 1
fi

if [[ "`$exec bibd.minion.bz2 -preprocess SSAC | grep ^SAC | print 3`" != "13" ]]; then
  echo SSAC test 1 failed
  exit 1
fi

if [[ "`$exec bibd.minion.bz2 -preprocess SSAC | grep ^SSAC | print 3`" != "54" ]]; then
  echo SSAC test 2 failed
  exit 1
fi

