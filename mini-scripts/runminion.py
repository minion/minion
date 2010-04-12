#!/usr/bin/python
import os, getopt, sys
import random

# very simple program, just runs minion on the given minion files, using tableout.

#Get the minion binary, and possibly a filelist and other options.
(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["sample=", "minion=", "filelist=", "benchdir=", "timelimit=", "tableout=", "nodelimit=", "args=", "xgrid"])

rand=random.randint(0, 1000000)
minion="bin/minion"
tableout="miniontable"
filelist=[]
timeout="-timelimit 1200 "
nodelimit=""
xgrid=False
args=""
sample=1

for (ident, value) in optargs:
    if ident=="--minion":
        minion=value
    elif ident=="--filelist": 
        filelist+=open(value, "r").readlines()
    elif ident=="--benchdir":
        print "benchdir=" + value
        for root, dirs, files in os.walk(value):
            for name in files:
                if name.endswith('.minion') or name.endswith('.minion.bz2'):
                    filelist.append(os.path.join(root,name))
    elif ident=="--timelimit":
        timeout="-timelimit "+str(value)+" "
    elif ident=="--tableout":
        tableout=value
    elif ident=="--nodelimit":
        nodelimit="-nodelimit "+str(value)+" "
    elif ident=="--xgrid":
        xgrid=True
    elif ident=="--args":
        args=" "+value+" "
    elif ident=="--sample":
        sample=int(value)

# grab any command line arguments which are minion instance files.
othercopy=other[:]
for l in othercopy:
    if l.endswith(".minion"):
        other.remove(l)
        filelist.append(l)

if len(other)!=1:
    print "Usage: runminion.py [--minion=<location of minion binary>]"
    print "       [--filelist=... | --benchdir=...] [--timelimit=...] [--tableout=..]"
    print "       instance.minion ... "
    print " Any number of instance directories, instances and filelists can be specified, they"
    print " will all be used."
    sys.exit(1)

# This script assumes it can use the current directory for temporary files.
assert filelist!=[] , "You need to specify some instance files in some way"
print filelist

if xgrid:
    os.system("export XGRID_CONTROLLER_HOSTNAME=xgrid.cs.st-andrews.ac.uk")
    os.system("export XGRID_CONTROLLER_PASSWORD=....")

for i in filelist:
    minioncommand1=minion+args+" -tableout "+tableout+" "+timeout+nodelimit+" "+i+" >>1."+str(rand)
    if xgrid:
        minioncommand1="xgrid -job submit "+minioncommand1
    print "Executing command:"+minioncommand1
    for sam in range(sample):
        status1=os.system(minioncommand1)
        assert status1 == 0 , "A minion exited in non-standard way on instance %s"%i

#os.system("mini-scripts/plot-comparison.pl 1.%d 2.%d --name1=%s --name2=%s"%(rand, rand, name1, name2))

