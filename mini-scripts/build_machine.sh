#!/bin/bash
name=`pwd`/machines/$1.$2.$3
cd `dirname $0`
killall gap 
time ( cd ../python-scscp; gap.sh myserver.g  > $name.gapout 2>&1)  &> $name.gap_time &
gap_PID=$!
sleep 10
time (./create_table_propagator.py $* > $name.vm_out 2>&1 ) &> $name.python_time
killall gap