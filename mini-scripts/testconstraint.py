#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
import random

random.seed(12345)   # stupid seed but at least it makes the test repeatable.

sys.setrecursionlimit(5000)

class testmodulo:
    def printtable(self, domains): 
        out=[]
        # given a list of lists for the domains, generate the table for modulo 
        numtuples=0
        for i in domains[0]:
            for j in domains[1]:
                if(i%j in domains[2]):
                    out.append([i, j, i%j])
        return out
        
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("modulo", True, reify, reifyimply, [3], ["posnum"], [1,1,1], self, False)

class testgacelement:
    def printtable(self, domains): 
        out=[]
        # given a list of lists for the domains, generate the table for gacelement 
        numvars=len(domains)
        vectordoms=domains[:-2]
        otherdoms=domains[-2:]
        tocross=domains[:-1]
        cross=[]
        crossprod(tocross, [], cross)
        for l in cross:
            if l[-1] >=0 and l[-1]<(len(l)-1):
                if l[l[-1]] in otherdoms[1]:
                    out.append(l+[l[l[-1]]])
                    
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("gacelement", False, reify, reifyimply, [6,1,1], ["smallnum", "num", "num"], [6,1,1], self, True)

class testwatchelement(testgacelement):
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("watchelement", False, reify, reifyimply, [6,1,1], ["smallnum", "num", "num"], [6,1,1], self, True)

class testelement(testgacelement):
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("element", False, reify, reifyimply, [6,1,1], ["smallnum", "num", "num"], [6,1,1], self, False)

class testalldiff:
    def printtable(self, domains):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            isalldiff=True
            for e in l:
                if l.count(e)>1:
                    isalldiff=False
            if not isalldiff:
                continue
            out.append(l)
        return out
        
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("alldiff", True, reify, reifyimply, [6], ["quitesmallnum"], [6], self, False)

class testalldiffgacslow(testalldiff):
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("alldiffgacslow", False, reify, reifyimply, [6], ["quitesmallnum"], [6], self, True)

class testdiseq(testalldiff):
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("diseq", True, reify, reifyimply, [2], ["num"], [1,1], self, False)

class testeq:
    # printtable essentially sets up pairsame constraint. negation of alldiff.
    def printtable(self, domains):
        cross=[]
        out=[]
        crossprod(domains, [], cross)
        for l in cross:
            isalldiff=True
            for e in l:
                if l.count(e)>1:
                    isalldiff=False
            if not isalldiff:
                out.append(l)
        return out
        
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("eq", True, reify, reifyimply, [2], ["num"], [1,1], self, False)

class testineq:
    def printtable(self, domains):
        const=self.const1
        #  x1 <= x2+const 
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        numtuples=0
        for l in cross:
            if l[0] <= (l[1]+const):
                out.append(l)
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("ineq", True, reify, reifyimply, [2], ["num"], [1,1,"const"], self, False)

class testlexleq:
    def printtable(self, domains, less=False):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            l1=l[:len(l)/2]
            l2=l[len(l)/2:]
            
            if (not less and l1 <= l2) or (less and l1<l2):
                out.append(l)
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("lexleq", True, reify, reifyimply, [4,4], ["quitesmallnum", "quitesmallnum"], [4,4], self, False)

class testlexless(testlexleq):
    def printtable(self, domains):
        return testlexleq.printtable(self, domains, less=True)
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("lexless", True, reify, reifyimply, [4,4], ["quitesmallnum", "quitesmallnum"], [4,4], self, False)

class testmax:
    def printtable(self, domains, ismax=True):
        # assume last var is the max var. and first is the reify var if relevant.
        cross=[]
        crossprod(domains[:-1], [], cross)
        out=[]
        for l in cross:
            if ismax:
                maxormin=max(l)
            else:
                maxormin=min(l)
            if maxormin in domains[-1]:
                out.append(l+[maxormin])
        
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("max", True, reify, reifyimply, [5,1], ["smallnum", "num"], [5,1], self, False)
    
class testmin(testmax):
    def printtable(self, domains):
        return testmax.printtable(self, domains, ismax=False)
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("min", True, reify, reifyimply, [5,1], ["smallnum", "num"], [5,1], self, False)

class testoccurrence:
    def printtable(self, domains):
        cross=[]
        out=[]
        crossprod(domains, [], cross)
        for l in cross:
            if l.count(self.const1)==self.const2:
                out.append(l)
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        # note that the two constants generated may be completely inappropriate. e.g. -2 occurrences..
        return runtestgeneral("occurrence", False, reify, reifyimply, [6], ["smallnum"], [6, "smallconst", "smallconst"], self, False)

class testproduct:
    def printtable(self, domains): 
        out=[]
        for i in domains[0]:
            for j in domains[1]:
                if(i*j in domains[2]):
                    out.append([i, j, i*j])
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("product", True, reify, reifyimply, [3], ["num"], [1,1,1], self, False)

class testsumgeq:
    def printtable(self, domains, less=False, weights=[1,1,1,1,1,1,1,1]):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            sumval=0
            for i in zip(l[:-1], weights):
                sumval+=product(i)
            
            if (not less and sumval>=l[-1]) or (less and sumval<=l[-1]):
                out.append(l)
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("sumgeq", True, reify, reifyimply, [5,1], ["smallnum", "num"], [5,1], self, False)
    
class testsumleq(testsumgeq):
    def printtable(self, domains):
        return testsumgeq.printtable(self, domains, less=True)
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("sumleq", True, reify, reifyimply, [5,1], ["smallnum", "num"], [5,1], self, False)

## Come back to these two. BUG.
class testweightedsumgeq(testsumgeq):
    def runtest(self):
        (domlists, modvars, tablevars)=generatevariables([6,1], ["smallnum", "num"], True)
        # run the table instance
        weights=[random.randint(-2, 2) for i in range(6)]
        tuplelist=self.printtable(domlists, False, False, weights)
        constraint="table([x0, x1, x2, x3, x4, x5, x6], modtable)"
        retval1=runminion("infile1.minion", "outfile1", "bin/minion", tablevars, constraint, tuplelist)
        # run the modulo instance
        constraint="weightedsumgeq([%s], [x0, x1, x2, x3, x4, x5], x6)"%reduce(lambda x,y:str(x)+","+str(y),weights)
        retval2=runminion("infile2.minion", "outfile2", "bin/minion", modvars, constraint, False)
        
        # now compare
        if retval1!=0 or retval2!=0:
            print "Minion exit values for infile1.minion, infile2.minion: %d, %d"%(retval1, retval2)
            return False
        return comparetrees(False)  # tree subset

class testweightedsumleq(testsumgeq):
    def runtest(self):
        (domlists, modvars, tablevars)=generatevariables([6,1], ["smallnum", "num"], True)
        # run the table instance
        weights=[random.randint(-2, 2) for i in range(6)]
        tuplelist=self.printtable(domlists, False, True, weights)
        constraint="table([x0, x1, x2, x3, x4, x5, x6], modtable)"
        retval1=runminion("infile1.minion", "outfile1", "bin/minion", tablevars, constraint, tuplelist)
        # run the modulo instance
        constraint="weightedsumleq([%s], [x0, x1, x2, x3, x4, x5], x6)"%reduce(lambda x,y:str(x)+","+str(y),weights)
        retval2=runminion("infile2.minion", "outfile2", "bin/minion", modvars, constraint, False)
        
        # now compare
        if retval1!=0 or retval2!=0:
            print "Minion exit values for infile1.minion, infile2.minion: %d, %d"%(retval1, retval2)
            return False
        return comparetrees(False)  # tree subset

# also need reified versions of the two above.

class testminuseq:
    def printtable(self, domains):
        out=[]
        for i in domains[0]:
            if (-i) in domains[1]:
                out.append([i, -i])
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("minuseq", True, reify, reifyimply, [2], ["num"], [1,1], self, True)
        
class testlitsumeq:
    # does this constraint even exist??
    def printtable(self, domains):
        tosum=domains[:-1]
        sumvar=domains[-1]
        cross=[]
        out=[]
        crossprod(tosum, [], cross)
        for l in cross:
            if sum(l) in sumvar:
                out.append(l+[sum(l)])
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("litsumeq", True, reify, reifyimply, [4,1], ["smallnum", "num"], [4,1], self, False)

class testwatchsumgeq:
    def printtable(self, domains, less=False):
        const=self.const1
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            sumval=sum(l)
            if (not less and sumval>=const) or (less and sumval<=const):
                out.append(l)
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("watchsumgeq", True, reify, reifyimply, [5], ["boolean"], [5, "const"], self, True)
    
class testwatchsumleq(testwatchsumgeq):
    def printtable(self, domains):
        return testwatchsumgeq.printtable(self, domains, less=True)
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("watchsumleq", True, reify, reifyimply, [5], ["boolean"], [5, "const"], self, True)
        
class testwatchvecneq:
    def printtable(self, domains):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            if l[:len(l)/2]!=l[len(l)/2:]:
                out.append(l)
        return out
    
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("watchvecneq", True, reify, reifyimply, [8], ["smallnum"], [4,4], self, False)
        # does not quite do GAC. Is it supposed to?
    
class testpow:
    def printtable(self, domains):
        cross=[]
        crossprod(domains[:-1], [], cross)
        out=[]
        for l in cross:
            if l[0]**l[1] in domains[-1]:
                out.append(l+[l[0]**l[1]])
        return out
    def runtest(self, reify=False, reifyimply=False):
        return runtestgeneral("pow", True, reify, reifyimply, [1,1,1], ["posnum","posnum","posnum"], [1,1,1], self, False)
    
################################################################################
# 
#                    Utility functions begin here.
# 

def crossprod(domains, conslist, outlist):
    if domains==[]:
        outlist.append(conslist[:])
        return
    for i in domains[0]:
        ccopy=conslist[:]
        ccopy.append(i)
        crossprod(domains[1:], ccopy, outlist)
    return

def product(list):
    return reduce(lambda x,y:x*y, list)

# print the minion file header
def printminionfile(fileh, variables, constraint, tuplelist):
    # constraint parameter must include reify if required.
    fileh.write("MINION 3\n")
    fileh.write("\n**VARIABLES**\n")
    fileh.write(variables)
    if tuplelist:
        fileh.write("\n**TUPLELIST**\n")
        fileh.write(tuplelist)
    fileh.write("\n**CONSTRAINTS**\n")
    fileh.write(constraint)
    fileh.write("\n**EOF**")

def runtestgeneral(constraintname, boundsallowed, reify, reifyimply, varnums, vartypes, howprintvars, tablegen, treesame):
    # given tablevars, tuplelist for the table instance
    # and modvars and constrainttest for the other instance, construct the
    # instances.
    # 
    
    # a constraint schema looks like this:
    # constraintname
    # boundsallowed
    # reify      -- whether we will test the reified constraint.
    # reifyimply
    # vartypes, a list of variables to generate ["smallnum", "num" whatever]
    # varnums, a list of numbers of variables [5, 1]
    # a list describing how to print the vars e.g. [ 6, 1, "const"] will print a vector of 6, a single variable, and a constant.
    # reifyimply \in true, false and reify \in true, false.
    
    # tablegen, an object with a printtable function.
    
    # will shortly be adapted to do optimization problems at random.
    
    if reify or reifyimply:
        # add extra bool variable.
        varnums=[1]+varnums
        vartypes=["boolean"]+vartypes
    
    (domlists, modvars, tablevars)=generatevariables(varnums, vartypes, boundsallowed)
    
    curvar=0
    if reify or reifyimply:
        curvar=1
    
    constraint=constraintname+"("
    
    constnum=1   # number of the current const.
    
    for i in howprintvars:
        if i=="const":
            const=random.randint(-20, 20)
            setattr(tablegen, "const%d"%constnum, const)
            constraint+="%d,"%const
            constnum+=1
        elif i=="smallconst":
            const=random.randint(-5, 5)
            setattr(tablegen, "const%d"%constnum, const)
            constraint+="%d,"%const
            constnum+=1
        elif i>1:
            #print vector
            constraint+="[x%d"%curvar
            curvar+=1
            for e in range(i-1):
                constraint+=", x%d"%curvar
                curvar+=1
            constraint+="],"
        else:
            assert i==1
            #print single variable.
            constraint+="x%d,"%curvar
            curvar+=1
    
    constraint=constraint[:-1]+")"   # hack off the last comma and put a bracket
    
    if reify:
        constraint="reify("+constraint+", x0)"
    if reifyimply:
        constraint="reifyimply("+constraint+", x0)"
    
    opt=random.randint(0,1)
    optvar=random.randint(0, sum(varnums))
    
    if reify or reifyimply:
        tuplelist=tablegen.printtable(domlists[1:])
    else:
        tuplelist=tablegen.printtable(domlists)
    
    if reify:
        tuplelist2=[]
        cross=[]
        crossprod(domlists[1:], [], cross)
        for c in cross:
            if c in tuplelist:
                tuplelist2.append([1]+c)
            else:
                tuplelist2.append([0]+c)
        tuplelist=tuplelist2
    if reifyimply:
        tuplelist2=[]
        cross=[]
        crossprod(domlists[1:], [], cross)
        for c in cross:
            if c in tuplelist:
                tuplelist2.append([1]+c)
                tuplelist2.append([0]+c)   # compatible with both values of the reification var.
            else:
                tuplelist2.append([0]+c)
        tuplelist=tuplelist2
    
    # now convert tuplelist into a string.
    tuplestring="modtable %d %d \n"%(len(tuplelist), sum(varnums))
    for l in tuplelist:
        for e in l:
            tuplestring+="%d "%e
        tuplestring+="\n"
    
    # tuplelist is actually a set of lists(not yet), so that it can be reformed for reify or reifyimply
    
    constrainttable="table([x0"
    for i in range(1,sum(varnums)):
        constrainttable+=",x%d"%i
    constrainttable+="], modtable)"
    
    retval1=runminion("infile1.minion", "outfile1", "bin/minion", tablevars, constrainttable, tuplestring)
    retval2=runminion("infile2.minion", "outfile2", "bin/minion", modvars, constraint, False)
    if retval1!=0 or retval2!=0:
        print "Minion exit values for infile1.minion, infile2.minion: %d, %d"%(retval1, retval2)
        return False
    if boundsallowed and modvars.rfind("SPARSEBOUND")!=-1:   
        # This is a temporary hack while sparsebound does not print its domain properly.
        treesame=False
    return comparetrees(treesame)  # tree subset

def runminion(filename, outfilename, minionbin, variables, constraint, tuplelist):
    file1=open(filename, "w")
    printminionfile(file1, variables, constraint, tuplelist)
    file1.close()
    cmd=minionbin+" -dumptree -findallsols "+filename+" >"+outfilename
    print "Executing command: "+cmd
    return os.system(cmd)

def generatevariables(varblocks, types, boundallowed):
    # generate a set of variables with random domains and types
    # Varblocks specifies the groups which should be of the same type
    # sum(varblocks) is the total number of variables.
    # types specifies num, posnum, nonnegnum or boolean for each block
    # boundallowed specifies if it is appropriate to have bound variables.
    st_nontable=""
    st_table=""
    domainlists=[]
    typesinczero=["num", "smallnum", "quitesmallnum",  "nonnegnum", "boolean"]
    for i in range(len(varblocks)):
        # randomly choose the type of the variables
        if boundallowed:
            if types[i] in typesinczero:
                t=random.randint(1,4)  # bound sparsebound bool or discrete
            else:
                t=random.randint(1,3)  # must be of type posnum, so not allowed to be a bool.
        else:
            if types[i] in typesinczero:
                t=random.randint(1,2)*2   # discrete or bool
            else:
                t=2   # can't be bool, must be discrete.
        
        if t==1:
            ty="BOUND "
        elif t==2:
            ty="DISCRETE "
        elif t==3:
            ty="SPARSEBOUND "  # Should be sparsebound
        else:
            ty="BOOL "
        
        if types[i]=="boolean":  # This is a hack put in place until reify is fixed, if ever.
            ty="BOOL "          # reify refuses to work unless the reification var is a bool.
        
        for j in range(varblocks[i]):
            # now for each variable generate a domain
            if types[i]=="num":
                lb=random.randint(-20, 0)
                ub=random.randint(1, 20)
            elif types[i]=="smallnum":
                lb=random.randint(-2, 0)
                ub=random.randint(0, 2)
            elif types[i]=="quitesmallnum":
                lb=random.randint(-4, 0)
                ub=random.randint(1, 4)
            elif types[i]=="verysmallnum":
                lb=random.randint(-2, 0)   # max 4 vals in domain.
                ub=random.randint(0, 1)
            elif types[i]=="posnum":
                lb=random.randint(1, 20)
                ub=random.randint(21, 40)
            elif types[i]=="nonnegnum":
                lb=random.randint(0, 20)
                ub=random.randint(21, 40)
            else:
                assert types[i]=="boolean"
                lb=0
                ub=1
            varnum=sum(varblocks[0:i])+j
            if ty=="SPARSEBOUND ":
                # take a random subset between lb and ub
                dom=random.sample(range(lb, ub+1), max((ub-lb)/2, 1))
                dom.sort(cmp)
                domainlists.append(dom)
                strdom=""
                for j in dom[:-1]:
                    strdom+=str(j)+","
                strdom+=str(dom[-1])
                st_nontable+=ty+"x%d {%s}\n"%(varnum, strdom)
                st_table+="DISCRETE x%d {%d..%d}\n"%(varnum, lb, ub)
            elif ty=="BOOL ":
                st_nontable+=ty+"x%d\n"%(varnum)
                st_table   +=ty+"x%d\n"%(varnum)
                domainlists.append([0,1])
            elif ty=="BOUND ":
                st_nontable+=ty+"x%d {%d..%d}\n"%(varnum, lb, ub)
                st_table+="DISCRETE x%d {%d..%d}\n"%(varnum, lb, ub)
                domainlists.append(range(lb, ub+1))
            else:
                st_nontable+=ty+"x%d {%d..%d}\n"%(varnum, lb, ub)
                st_table   +=ty+"x%d {%d..%d}\n"%(varnum, lb, ub)
                domainlists.append(range(lb, ub+1))
    # co-sort the lines of st_nontable and st_table
    # This is to avoid the problem of variables coming out in different
    # orders in minion's dumptree output.
    deco=zip(st_nontable.strip().split("\n"), st_table.strip().split("\n"))
    
    def comparefunc(x,y):   # sort in order: bool, bound, sparsebound, discrete
        t1=x[0][:4]
        t2=y[0][:4]
        dic={"BOOL":1, "BOUN":2, "SPAR":3, "DISC":4}
        return dic[t1]-dic[t2]
    
    deco.sort(cmp=comparefunc)
    st_nontable=reduce(lambda x,y:x+"\n"+y, [i[0] for i in deco])+"\n"
    st_table=reduce(lambda x,y:x+"\n"+y, [i[1] for i in deco])+"\n"
    return (domainlists, st_nontable, st_table)

def comparetrees(same):
    # Fork so that memory used in building and comparing
    # trees in the child process is returned when the process ends.
    r, w = os.pipe() # these are file descriptors, not file objects
    
    pid = os.fork()
    if pid:
        # we are the parent
        os.close(w) # use os.close() to close a file descriptor
        r = os.fdopen(r) # turn r into a file object
        txt = r.read()
        os.waitpid(pid, 0) # make sure the child process gets cleaned up
        return bool(txt)
    else:
        # we are the child
        os.close(r)
        w = os.fdopen(w, 'w')
        
        # do the tree comparison:
        tree1=tree()
        tree2=tree()
        
        tree1.buildtree("outfile1")
        tree2.buildtree("outfile2")
        
        if same:
            temp= tree1.equal(tree2)
        else:
            temp= tree1.subset(tree2)
        
        # now return 
        if temp:
            w.write("True")
        
        w.close()
        sys.exit(0)


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
    
