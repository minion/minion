#!/bin/bash

# arguments are the minion output files to get the times from

for i in $*; do
    if [ -n $SYSTIME_TOO ]; then
        time=`grep "Total Time: " $i | grep -o "[0-9.]\+"`
        time=$time" "`grep "Total System Time: " $i | grep -o "[0-9.]\+"`
    else
        time=`grep "Total Time: " $i | grep -o "[0-9.]\+"`
    fi
    nodes=`grep "Total Nodes: " $i | grep -o "[0-9]\+"`
    echo "$i $time"
    echo "$i $nodes" 1>&2
done
