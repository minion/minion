#!/bin/bash

case $1 in
    setuptime)
        grep "Setup Time:" | awk '{print $3}'
        ;;
    solutions)
        grep "Solutions Found:" | awk '{print $3}'
        ;;
    solvetime)
        grep "Solve Time:" | awk '{print $3}'
        ;;
    nodes)
        grep "Total Nodes:" | awk '{print $3}'
        ;;
    totaltime)
        grep "Total Time:" | awk '{print $3}'
        ;;
    *)
        echo 'Strip one piece of info out of a minion output'
        echo 'get_info.sh (setuptime|solutions|solvetime|nodes|totaltime)'
esac