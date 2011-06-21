#!/usr/bin/python
import os, getopt, sys
import random

#Get two minion binaries, and possibly a filelist and other options.
(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["minion1=", "minion2=", "filelist=", "benchdir=", "xgrid", "name1=", "name2="])

rand=random.randint(0, 1000000)

name1=False
name2=False

filelist=[]
timeout="-timelimit 1200"

for (ident, value) in optargs:
    if ident=="--minion1":
        minion1=value
    elif ident=="--minion2":
        minion2=value
    elif ident=="--filelist": 
        filelist+=open(value, "r").readlines()
    elif ident=="--benchdir":
        print "benchdir=" + value
        for root, dirs, files in os.walk(value):
            for name in files:
                if name.endswith('.minion'):
                    filelist.append(os.path.join(root,name))
    elif ident=="--xgrid":
        print "xgrid not implemneted"
    elif ident=="--timelimit":
        timeout="-timelimit "+str(value)
    elif ident=="--name1":
        name1=value
    elif ident=="--name2":
        name2=value

if not name1:
    name1=minion1
if not name2:
    name2=minion2

# grab any command line arguments which are minion instance files.
othercopy=other[:]
for l in othercopy:
    if l.endswith(".minion"):
        other.remove(l)
        filelist.append(l)

if len(other)!=1:
    print "Usage: regression-test.py [--minion1=<location of minion binary>] [--minion2=...]"
    print "       [--filelist=... | --benchdir=...] [--xgrid] [--timelimit=...]"
    print "       instance.minion ... "
    print " Any number of instance directories, instances and filelists can be specified, they"
    print " will all be used."
    sys.exit(1)

# This script assumes it can use the current directory for temporary files.
assert filelist!=[] , "You need to specify some instance files in some way"
print filelist

if os.path.exists("tablefile1"):
    os.remove("tablefile1")
if os.path.exists("tablefile2"):
    os.remove("tablefile2")

for i in filelist:
    minioncommand1=minion1+" -tableout tablefile1 "+timeout+" "+i+" >>1."+str(rand)
    minioncommand2=minion2+" -tableout tablefile2 "+timeout+" "+i+" >>2."+str(rand)
    print "Executing command:"+minioncommand1
    status1=os.system(minioncommand1)
    if status1==0:
        print "Executing command:"+minioncommand2
        status2=os.system(minioncommand2)
    
    assert status1+status2 == 0 , "A minion exited in non-standard way on instance %s"%i

os.system("mini-scripts/plot-comparison.pl 1.%d 2.%d --name1=%s --name2=%s"%(rand, rand, name1, name2))

