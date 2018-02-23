#!/bin/bash


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

if [[ "`$exec bibd.minion -preprocess SAC | grep ^SAC | awk '{print $3}'`" != "36" ]]; then
  echo SAC test failed
  exit 1
fi

if [[ "`$exec bibd.minion -preprocess SSAC | grep ^SAC | awk '{print $3}'`" != "36" ]]; then
  echo SSAC test 1 failed
  exit 1
fi

if [[ "`$exec bibd.minion -preprocess SSAC | grep ^SSAC | awk '{print $3}'`" != "244" ]]; then
  echo SSAC test 2 failed
  exit 1
fi

if [[ "`$exec meb-inst-18-09.eprime-param.minion  -nodelimit 50000 | grep 'Value: ' | awk '{print $2}'`" != "-1045," ]]; then
  echo Neighbourhood test failed
  exit 1
fi