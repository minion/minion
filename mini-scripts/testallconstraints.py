#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
from multiprocessing import Pool, Manager
import random
#from sendemail import *
import time

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["minion=", "numtests=", "email", "fullprop", "64bit", "procs=", "seed=", "conslist="])

if len(other)>1:
    print("Usage: testallconstraints.py [--minion=<location of minion binary>] [--numtests=...] [--email] [--procs=...] [--seed=...] [--conslist=...]")
    sys.exit(1)

# This one tests all the constraints in the following list.
conslist=[]


# equality constraints
conslist+=["diseq", "eq", "gaceq"]

# alldiffs
conslist+=["alldiff", "gacalldiff"]

# capacity constraints
conslist+=["gcc", "gccweak", "occurrence", "occurrenceleq", "occurrencegeq"]

#element constraints
conslist+=["element", "gacelement-deprecated", "watchelement"]

conslist+=["watchelement_one", "element_one"]

# arithmetic constraints
conslist+=["modulo", "modulo_undefzero", "pow", "minuseq", "product", "div", "div_undefzero", "abs"]

conslist+=["watchsumleq", "watchsumgeq", "watchvecneq", "staticvecneq", "hamming", "not-hamming"]
conslist+=["weightedsumleq", "weightedsumgeq"]

conslist+=["litsumgeq"]  

# should test table to test reifytable? and reifyimplytable

conslist+=["sumgeq", "sumleq", "weightedsumleq", "weightedsumgeq"]
conslist+=["ineq"]
conslist+=["difference"]

conslist+=["negativetable", "lighttable"]

# symmetry-breaking constraints

conslist+=["lexleq", "lexless", "lexleq_rv", "lexleq_quick", "lexless_quick"]

conslist+=["max", "min"]

conslist+=["watchneq", "watchless"]

conslist+=["w-inset", "w-notinset", "w-inrange", "w-notinrange", "w-literal", "w-notliteral"]

conslist+=["watchsumgeq", "litsumgeq", "watchneq", "watchless", "not-hamming"]
conslist+=["not-hamming"]

conslist+=["gacschema", "haggisgac", "haggisgac-stable", "str", "shortstr"]
# add reifyimply variant of all constraints,
# and reify variant of all except those in reifyexceptions
it=conslist[:]
for c in it:
        conslist+=["reifyimply"+c]
        conslist+=["reify"+c]

numtests=100
minionbin="bin/minion"
email=False
fullprop=False   # compare the constraint against itself with fullprop. Needs DEBUG=1.
bit64=False
procs=1
seed=12345
for i in optargs:
    (a1, a2)=i
    if a1=="--minion":
        minionbin=a2
    elif a1=="--numtests":
        numtests=int(a2)
    elif a1=="--email":
        email=True
    elif a1=="--fullprop":
        fullprop=True
    elif a1=="--64bit":
        bit64=True
    elif a1=="--procs":
        procs=int(a2)
    elif a1=="--seed":
        seed=int(a2)
    elif a1=="--conslist":
        conslist=a2.split(",")

def runtest(consname):
    cachename = consname
    starttime=time.time()
    sys.stdout.flush()
    random.seed(seed)
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
        options = {'reify': reify, 'reifyimply': reifyimply, 'fullprop': fullprop, 'printcmd': False, 'fixlength':False, 'getsatisfyingassignment':True}
        if not testobj.runtest(options):
            print("Failed when testing %s"%cachename)
            sys.stdout.flush()
            return False
    print("Completed testing %s, duration: %d"%(cachename, time.time()-starttime))
    return True

if __name__ == '__main__':

    p = Pool(procs)
    retval = p.map(runtest, conslist)

    if all(retval):
        print("Success")
        exit(0)
    else:
        print("Failure")
        exit(1)
    

