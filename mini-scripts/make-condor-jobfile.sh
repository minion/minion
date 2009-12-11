#!/bin/bash

# first argument is the name of the binary
# subsequent arguments are paths to the instances
# set $MACHINE to a FQDN if you want the job to run on a specific machine
# set $NUM to the number of times you want to run each instance (default 5)
# to pass arguments use $ARGUMENTS, e.g. ARGUMENTS="firstparam 'second param'" ./make-condor-jobfile.sh...

echo "Universe = Vanilla"
echo "should_transfer_files = YES"
echo "when_to_transfer_output = ON_EXIT"
echo -n "requirements = (Arch == \"X86_64\")"
if [ "X"$MACHINE != "X" ]; then
    echo " && (machine == \"$MACHINE\")"
else
    echo ""
fi

echo "Notification = Never"

BIN=$1
shift
BBIN=`basename $BIN`

echo "Executable = $BIN"

echo ""

NUMBER=5
if [ "X"$NUM != "X" ]; then
    NUMBER=$NUM
fi

for i in $*; do
    echo "input = $i"
    TMP=`basename $i`
    echo "arguments = \" $ARGUMENTS $TMP\""
    echo "output = $BBIN-$TMP.out.\$(Cluster).\$(Process)"
    echo "Queue $NUMBER"
    echo ""
    shift
done
