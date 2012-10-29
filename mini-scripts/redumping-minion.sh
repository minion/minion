#!/bin/bash
tmpfile=$(mktemp)
echo $* > W
$MINION_EXEC $* -outputCompressed $tmpfile > /dev/null && shift && $MINION_EXEC $* $tmpfile && rm $tmpfile