#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
import random
from sendemail import *
from string import split

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["minion=", "numtests=", "email", "fullprop", "64bit", "procs=", "seed=", "conslist="])

if len(other)>1:
    print "Usage: testallconstraints.py [--minion=<location of minion binary>] [--numtests=...] [--email] [--procs=...] [--seed=...] [--conslist=...]"
    sys.exit(1)

# This one tests all the constraints in the following list.
conslist=[]


# equality constraints
conslist+=["diseq", "eq"]

# alldiffs
conslist+=["alldiff", "gacalldiff"]

# capacity constraints
conslist+=["gcc", "gccweak", "occurrence", "occurrenceleq", "occurrencegeq"]

#element constraints
conslist+=["element", "gacelement-deprecated", "watchelement"]

conslist+=["watchelement_one", "element_one"]

# arithmetic constraints
conslist+=["modulo", "pow", "minuseq", "product", "div", "abs"]

conslist+=["watchsumleq", "watchsumgeq", "watchvecneq", "hamming", "not-hamming"]
conslist+=["weightedsumleq", "weightedsumgeq"]

conslist+=["litsumgeq"]  

# should test table to test reifytable? and reifyimplytable

conslist+=["sumgeq", "sumleq", "weightedsumleq", "weightedsumgeq"]
conslist+=["ineq"]
conslist+=["difference"]

# symmetry-breaking constraints

conslist+=["lexleq", "lexless", "lexleq_quick", "lexless_quick"]

conslist+=["max", "min"]

conslist+=["watchneq", "watchless"]

reifyexceptions=["watchsumgeq", "litsumgeq", "watchneq", "watchless", "not-hamming"]
reifyimplyexceptions=["not-hamming"]
# add reifyimply variant of all constraints,
# and reify variant of all except those in reifyexceptions
it=conslist[:]
for c in it:
    if c not in reifyimplyexceptions:
        conslist+=["reifyimply"+c]
    if c not in reifyexceptions:
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
        conslist=split(a2, ",")

workers = []
readers = []
for procNum in range(procs):
    if procNum == procs - 1:
        num = (len(conslist) // procs) + (len(conslist) % procs)
    else:
        num = (len(conslist) // procs)
    r, w = os.pipe()
    pid = os.fork()
    if pid:
        workers.append(pid)
        readers.append(r)
        os.close(w)
    else:
        os.close(r)
        sys.stdout = os.fdopen(w, 'w')
        offset = procNum * (len(conslist) // procs)
        for consname1 in conslist[offset:(offset + num)]:
            print "Testing %s, seed %i"%(consname1, seed)
            sys.stdout.flush()
            random.seed(seed)
        
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
                options = {'reify': reify, 'reifyimply': reifyimply, 'fullprop': fullprop, 'printcmd': False}
                if not testobj.runtest(options):
                    print "Failed when testing %s"%consname1
                    sys.stdout.flush()
                    sys.exit(1)
        sys.stdout.close()
        sys.exit(0)

for worker, reader in zip(workers, readers):
    read = os.fdopen(reader)
    (pid, exitcode) = os.waitpid(worker, 0)
    sys.stdout.write(read.read())
    read.close()
    if exitcode != 0:
        if email:
            mailstring="Mail from testallconstraints.py.\n"
            mailstring+="Problem with constraint %s. Run testconstraint.py %s on current SVN to replicate the test.\n"%(consname1, consname1)
            if fullprop:
                mailstring+="Testing equivalence of -fullprop and normal propagation.\n"
            else:
                mailstring+="Testing correctness against table representation.\n"
            if bit64:
                mailstring+="Testing 64bit variant.\n"
            mailstring+="Using binary %s\n"%minionbin
            mail(mailstring)
        print "Test failed"
        sys.exit(1)

print "Test succeeded"
# if we got here, send an email indicating success.
if email:
    mailstring="Mail from testallconstraints.py.\n"
    mailstring+="Using binary %s\n"%minionbin
    mailstring+="Tested the following constraints with no errors.\n"
    mailstring+=str(conslist)
    if bit64:
        mailstring+="Testing 64bit variant.\n"
    
    mail(mailstring, subject="Minion test successful.")
    

