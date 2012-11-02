#!/bin/bash
tmpfile=$(mktemp)
$MINION_EXEC $* -outputCompressed $tmpfile > /dev/null && shift && $MINION_EXEC $* $tmpfile && rm $tmpfile