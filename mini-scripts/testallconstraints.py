#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
import random
from sendemail import *

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["minion=", "numtests=", "email"])

if len(other)>1:
    print "Usage: testallconstraints.py [--minion=<location of minion binary>] [--numtests=...] [--email]"
    sys.exit(1)

# This one tests all the constraints in the following list.
conslist=[]


# equality constraints
conslist+=["diseq", "reifydiseq", "reifyimplydiseq", "eq", "reifyeq", "reifyimplyeq"]

# alldiffs
conslist+=["alldiff", "reifyalldiff",  "reifyimplyalldiff", "alldiffgacslow", "reifyalldiffgacslow", "reifyimplyalldiffgacslow"]

#element constraints
conslist+=["gacelement-deprecated", "reifyimplygacelement-deprecated"]
conslist+=["element", "reifyimplyelement", "watchelement"]

#non-reifiable arithmetic constraints
conslist+=["modulo", "reifyimplymodulo", "pow", "reifyimplypow", "minuseq", "reifyimplyminuseq"]
conslist+=["product", "reifyimplyproduct"]
conslist+=["div", "reifyimplydiv"]

conslist+=["watchsumleq", "watchsumgeq", "watchvecneq", "watchvecexists_less", "watchvecexists_and", "hamming"]

conslist+=["sumgeq", "reifysumgeq", "reifyimplysumgeq"]
conslist+=["sumleq", "reifysumleq", "reifyimplysumleq"]

conslist+=["occurrence", "reifyimplyoccurrence"]
conslist+=["occurrenceleq", "occurrencegeq"]

conslist+=["ineq", "reifyineq", "reifyimplyineq"]

# symmetry-breaking constraints

conslist+=["lexleq", "lexless", "reifylexleq", "reifylexless", "reifyimplylexleq", "reifyimplylexless"]

conslist+=["max", "min", "reifyimplymax", "reifyimplymin"]



#todo
#conslist+=["weightedsumleq"...

numtests=100
minionbin="bin/minion"
email=False
for i in optargs:
    (a1, a2)=i
    if a1=="--minion":
        minionbin=a2
    elif a1=="--numtests":
        numtests=int(a2)
    elif a1=="--email":
        email=True

for consname1 in conslist:
    random.seed(12345)   # stupid seed but at least it makes the test repeatable.
    
    reify=False
    reifyimply=False
    consname=consname1
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
        print "Test number %d"%(testnum)
        if not testobj.runtest(reify=reify, reifyimply=reifyimply):
            if email:
                mailstring="Mail from testallconstraints.py.\n"
                mailstring+="Problem with constraint %s. Run testconstraint.py %s on current SVN to replicate the test.\n"%(consname1, consname1)
                mailstring+="Using binary %s\n"%minionbin
                mail(mailstring)
            sys.exit(1)

# if we got here, send an email indicating success.
if email:
    mailstring="Mail from testallconstraints.py.\n"
    mailstring+="Using binary %s\n"%minionbin
    mailstring+="Tested the following constraints with no errors.\n"
    mailstring+=str(conslist)
    mail(mailstring, subject="Minion test successful.")
    

