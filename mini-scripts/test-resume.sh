#!/bin/bash

MINION=$1

for i in ./test_instances/resume_instances/*.minion; do

    INSTANCE=$i;
    echo $i;

    COMPLETEOUTPUT=`mktemp test-resume.co.XXXXXX`;
    FIRSTPARTIALOUTPUT=`mktemp test-resume.fpo.XXXXXX`;
    SECONDPARTIALOUTPUT=`mktemp test-resume.spo.XXXXXX`;
    
    $MINION $INSTANCE > $COMPLETEOUTPUT;
    completenodes=`grep "Total Nodes" $COMPLETEOUTPUT | cut -d' ' -f3`;
    completesols=`grep "Solutions Found" $COMPLETEOUTPUT | cut -d' ' -f3`;
    completewalltime=`grep "Total Wall Time" $COMPLETEOUTPUT | cut -d' ' -f4`;
    completesolvetime=`grep "Solve Time" $COMPLETEOUTPUT | cut -d' ' -f3`;
    
    #don't test if the solve time is under a second, because  it is difficult to ensure
    #that the first partial run later will not run to competition, due to variation in
    #solve times
    continue=`echo "($completewalltime - $completesolvetime)<1.0" | bc`;
    if [ $continue -eq 1 ]; then
	echo "Solve time under a second - don't test";
	continue;
    fi

    timeout=`echo "scale=9;(($completewalltime+$completesolvetime)/2)" | bc`;
    
    echo complete nodes$completenodes sols$completesols walltime$completewalltime solvetime$completesolvetime timeout$timeout;
    
    $MINION $INSTANCE > $FIRSTPARTIALOUTPUT &
    PID=$!;
    sleep $timeout;
    kill -2 $PID;
    sync; #i have found that this is necessary to ensure the data can be extracted properly
    resumefile=`grep "Output resume file" $FIRSTPARTIALOUTPUT | cut -d' ' -f5 `;
    resumefile=${resumefile#\"}; #remove leading quote
    resumefile=${resumefile%\"}; #remove trailing quote
    firstpartialnodes=`grep "Total Nodes" $FIRSTPARTIALOUTPUT | cut -d' ' -f3`;
    firstpartialsols=`grep "Solutions Found" $FIRSTPARTIALOUTPUT | cut -d' ' -f3`;
    echo first pid$PID res$resumefile nodes$firstpartialnodes sols$firstpartialsols;
    
    $MINION -resume-file $resumefile $INSTANCE > $SECONDPARTIALOUTPUT;
    secondpartialnodes=`grep "Total Nodes" $SECONDPARTIALOUTPUT | cut -d' ' -f3`;
    secondpartialsols=`grep "Solutions Found" $SECONDPARTIALOUTPUT | cut -d' ' -f3`;
    echo second nodes$secondpartialnodes sols$secondpartialsols;
    
    echo "Complete nodes: $completenodes";
    echo "Resume nodes: $(($firstpartialnodes+$secondpartialnodes))=$firstpartialnodes+$secondpartialnodes";
    echo "Complete sols: $completesols";
    echo "Resume sols: $(($firstpartialsols+$secondpartialsols))=$firstpartialsols+$secondpartialsols";
    
    if [ $completesols -ne $(($firstpartialsols+$secondpartialsols)) ]; then
        exit 1; #fail and exit if sols don't match
    fi

    rm -f $COMPLETEOUTPUT $FIRSTPARTIALOUTPUT $SECONDPARTIALOUTPUT
done
