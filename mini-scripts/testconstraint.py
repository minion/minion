#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
import random

random.seed(12345)   # stupid seed but at least it makes the test repeatable.

if len(sys.argv)<2:
    print "Usage: testconstraint.py constraintname"

consname=sys.argv[1]  # name of the constraint to test

reify=False
reifyimply=False
if consname[0:10]=="reifyimply":
    reifyimply=True
    consname=consname[10:]

if consname[0:5]=="reify":
    reify=True
    consname=consname[5:]

testobj=eval("test"+consname+"()")

for testnum in range(10000):
    print "Test number %d"%(testnum)
    if not testobj.runtest(reify=reify, reifyimply=reifyimply):
        sys.exit(0)
    
