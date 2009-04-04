#!/bin/bash
# $1 is the revision number to make the release for
# $2 is the directory to do everything in
# $3 is the SVN revision to use
# if $4 is present, run the tests
if [ ! -d $2 ]; then
    mkdir $2
fi
cd $2

svn co -r $3 https://minion.svn.sourceforge.net/svnroot/minion/trunk minion-$1 || exit "Unable to check out minion source from sourceforge!"
cd minion-$1

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
mkdir bin && cd bin
(cmake -DSTATIC=1 -DNAME=minion .. && cmake -DSTATIC=1 -DNAME=minion .. && make minion) || "Failed to build minion"
(cmake -DSTATIC=1 -DDEBUG=1 -DUNOPTIMISED=1 -DNAME=minion-debug .. && cmake -DSTATIC=1 -DDEBUG=1 -DUNOPTIMISED=1 -DNAME=minion-debug .. && make minion-debug) || exit "Failed to build debug minion"
cd ..

if [ $# -ge 4 ]; then
    cd test_instances && (./run_tests.sh ../bin/minion \
        && ./run_tests.sh ../bin/minion-debug) || exit "Failed in run_tests.sh"
    cd .. && (./mini-scripts/testallconstraints.py --numtests=100 \
            --minion=bin/minion && ./mini-scripts/testallconstraints.py \
            --numtests=100 --minion=bin/minion-debug) || exit "Failed in testallconstraints.py"
fi

# save the parameters
echo $@ > RELEASE_PARAMS

cd ..

# tar everything
tar cf minion-$1-src.tar --exclude=*.pyc --exclude=.svn minion-$1/minion \
                         minion-$1/test_instances \
                         minion-$1/CMakeLists.txt \
                         minion-$1/mini-scripts \
                         minion-$1/benchmarks \
                         minion-$1/generators \
                         minion-$1/LICENSE.txt \
                         minion-$1/README \
                         minion-$1/docs/Manual.pdf \
                         minion-$1/docs/Manual.tex \
                         minion-$1/docs/general.bib \
                         minion-$1/docs/EightPuzzleDiagram.pdf \
                         minion-$1/docs/k4xp2.pdf \
                         minion-$1/RELEASE_PARAMS \
                         minion-$1/visualisation \
                         minion-$1/cmake-modules \
                         minion-$1/release-scripts &&
cp minion-$1-src.tar minion-$1.tar &&
tar rf minion-$1.tar minion-$1/bin/minion?(.exe) &&
cp minion-$1-src.tar minion-$1-debug.tar &&
tar rf minion-$1-debug.tar minion-$1/bin/minion-debug?(.exe) &&
gzip *.tar
