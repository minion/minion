#!/bin/bash
# $1 is the revision number to make the release for
# $2 is the directory to do everything in
# $3 is the SVN revision to use
# if $4 is present, run the tests
if [ ! -d $2 ]; then
    mkdir $2
fi
cd $2

svn co -r $3 https://minion.svn.sourceforge.net/svnroot/minion/trunk . || exit "Unable to check out minion source from sourceforge!"

# check the version of the manual
MANUALVER=`grep '\\\def\\\minionversion' docs/Manual.tex | awk -F '{' '{ print substr($2,0,length($2)-1); }'`
if [ -z $MANUALVER ]; then
    echo "Unable to determine manual version, proceeding anyway"
else
    if [ $1 != $MANUALVER ]; then
        echo "The release version you've requested is not the same as the version in the manual, hit <Ctrl-C> to abort, <Enter> to continue anyway."
        read
    fi
fi

# build standard and debug binaries
# We run cmake twice due to a bug on windows -- it should make no difference anywhere else!
(cmake -DSTATIC=1 -DNAME=minion . && cmake -DSTATIC=1 -DNAME=minion . && make minion) || "Failed to build minion"
(cmake -DSTATIC=1 -DDEBUG=1 -DUNOPTIMISED=1 -DNAME=minion-debug . && cmake -DSTATIC=1 -DDEBUG=1 -DUNOPTIMISED=1 -DNAME=minion-debug . && make minion-debug) || exit "Failed to build debug minion"

if [ $# -ge 4 ]; then
    cd test_instances && (./run_tests.sh ../bin/minion \
        && ./run_tests.sh ../bin/minion-debug) || exit "Failed in run_tests.sh"
    cd .. && (./mini-scripts/testallconstraints.py --numtests=100 \
            --minion=bin/minion && ./mini-scripts/testallconstraints.py \
            --numtests=100 --minion=bin/minion-debug) || exit "Failed in testallconstraints.py"
fi

# save the parameters
echo $@ > RELEASE_PARAMS

# tar everything
tar cf minion-$1-src.tar --exclude=.svn minion test_instances \
                         CMakeLists.txt mini-scripts benchmarks \
                         generators LICENSE.txt README docs/Manual.pdf \
                         docs/Manual.tex docs/general.bib \
                         docs/EightPuzzleDiagram.pdf docs/k4xp2.pdf \
                         RELEASE_PARAMS visualisation cmake-modules &&
cp minion-$1-src.tar minion-$1.tar &&
tar rf minion-$1.tar bin/minion &&
cp minion-$1-src.tar minion-$1-debug.tar &&
tar rf minion-$1-debug.tar bin/minion-debug &&
gzip *.tar
