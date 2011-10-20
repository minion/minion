#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
import random


(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["minion=", "numtests=", "fullprop", "seed="])

if len(other)!=2:
    print("Usage: testconstraint.py [--minion=<location of minion binary>] [--numtests=...] [--fullprop] constraintname")
    sys.exit(1)

numtests=10000
minionbin="bin/minion"
fullprop=False
seed=12345

for i in optargs:
    (a1, a2)=i
    if a1=="--minion":
        minionbin=a2
    elif a1=="--numtests":
        numtests=int(a2)
    elif a1=="--fullprop":
        fullprop=True
    elif a1=="--seed":
        seed=int(a2)

random.seed(seed)   # stupid seed but at least it makes the test repeatable.

consname=other[1]  # name of the constraint to test

reify=False
reifyimply=False
if consname[0:10]=="reifyimply":
    reifyimply=True
    consname=consname[10:]

if consname[0:5]=="reify":
    reify=True
    consname=consname[5:]

consname=consname.replace("-", "__minus__")
testobj=eval("test"+consname+"()")
testobj.solver=minionbin
for testnum in range(numtests):
    print("Test number %d"%(testnum))
    options={'reify':reify, 'reifyimply':reifyimply, 'fullprop':fullprop, 'printcmd':True, 'fixlength':False, 'getsatisfyingassignment':True}
    if not testobj.runtest(options):
        sys.exit(0)
    
