#!/bin/bash

# arguments are the minion output files to get the times from

for i in $*; do
    time=`grep "Total Time: " $i | grep -o "[0-9.]\+"`
    nodes=`grep "Total Nodes: " $i | grep -o "[0-9]\+"`
    echo "$i $time"
    echo "$i $nodes" 1>&2
done
