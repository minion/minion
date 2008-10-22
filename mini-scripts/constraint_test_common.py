import sys, os
from weakref import ref

def partition(splitter, st):
    # split the string on the first instance of splitter
    st2=st.split(splitter, 1)
    if len(st2)==1:
        return (st2[0], splitter, "")
    else:
        return (st2[0], splitter, st2[1])

class sol:
    def read(self, line):
        line=line.strip()
        self.solution=[int(num) for num in line.split(" ")]
    
    def equal(self, sol2):
        if not hasattr(sol2, "solution"):
            print "Spurious node (should be a solution) in tree 2: node %d" %sol2.nodenum
            return False
        if self.solution!=sol2.solution:
            print "Solutions different:"+str(self.solution)+" "+str(sol2.solution)
            return False
        else:
            return True
        
    def subset(self, sol2):
        # note that sol2 may be an internal node, not a solution.
        if hasattr(sol2, "solution"):
            return self.equal(sol2)
        # sol2 is a node.
        if not sol2.solutionbelow():
            print "Solution missing in tree 2 below node %d"%sol2.nodenum
            return False
        if hasattr(sol2, "left") and hasattr(sol2, "right") and sol2.left.solutionbelow() and sol2.right.solutionbelow():
            print "Too many solutions in tree 2 below node %d"%sol2.nodenum
            return False
        if hasattr(sol2, "left") and sol2.left.solutionbelow():
            return self.subset(sol2.left)
        else:
            assert hasattr(sol2, "right") and sol2.right.solutionbelow()
            return self.subset(sol2.right)
        
    def solutionbelow(self):
        #print "Superfluous solution: "+str(self.solution)
        return True
    
    def outputdot(self, fileh):
        fileh.write("s%d name=sol%d;\n"%(self.solnum, self.solnum))

class node:
    def read(self, lineorig):
        (number,temp,line)=partition(",", lineorig)
        self.nodenum=int(number)
        line=line.lstrip()
        # now onto the domains
        (tribracket, temp, line)=partition("<", line)
        (line, temp, temp2)=partition(">", line)
        line=line.strip()
        self.domains=dict()
        domaincount=1
        
        while len(line)>0:
            line=line.strip()
            if line[0].isdigit() or line[0]=="-":
                (num, temp, line)=partition(",", line)
                line=temp+line
                self.domains[domaincount]=[int(num)]
            else:
                assert line[0]=="{" or line[0]=="["
                if line[0]=="{":
                    (nums, temp, line)=partition("}", line[1:])
                    self.domains[domaincount]=[int(num) for num in nums.split(",")]
                else:
                    (nums, temp, line)=partition("]", line[1:])
                    bounds=[int(num) for num in nums.split(",")]
                    self.domains[domaincount]=range(bounds[0], bounds[1]+1)
                
            domaincount=domaincount+1
            if len(line)>0 and line[0]==",": line=line[1:]
            if False:
                print "stored domain "+str((domaincount-1))+" "+str(self.domains[domaincount-1])
        return True
    
    def equal(self, node2):
        # Compare the internal state here with the state of node2
        if self.domains!=node2.domains:
            print "Domains not equal at node %d"%self.nodenum
            return False
        if self.nodenum!=node2.nodenum:
            print "Different tree structure near nodes %d and %d"%(self.nodenum, node2.nodenum)
            return False
        if hasattr(self, "left")!=hasattr(node2, "left"):
            print "Different tree structure (left branch) after node "+str(self.nodenum)
            return False
        if hasattr(self, "right")!=hasattr(node2, "right"):
            print "Different tree structure (right branch) after node "+str(self.nodenum)
            return False
        if hasattr(self, "left"):
            if not self.left.equal(node2.left):
                print "False on left branch"
                return False
        if hasattr(self, "right"):
            if not self.right.equal(node2.right):
                print "False on right branch"
                return False
        return True
    
    def solutionbelow(self):
        flag=False
        if hasattr(self, "left"):
            flag= flag or self.left.solutionbelow()
        if hasattr(self, "right"):
            flag= flag or self.right.solutionbelow()
        return flag
        
    def outputdot(self, fileh):
        fileh.write("%d [label=\"Node%d\"];\n"%(self.nodenum, self.nodenum) )
        thisname=str(self.nodenum)
        
        if hasattr(self, "left"):
            if hasattr(self.left, "nodenum"):
                leftname=str(self.left.nodenum)
            else:
                leftname="s"+str(self.left.solnum)
            fileh.write("%s -> %s [label=\"%s\"];\n"% (thisname, leftname, ))
            self.left.outputdot(fileh)
        if hasattr(self, "right"):
            if hasattr(self.right, "nodenum"):
                rightname=str(self.right.nodenum)
            else:
                rightname="s"+str(self.right.solnum)
            fileh.write("%s -> %s;\n"% (thisname, rightname))
            self.right.outputdot(fileh)
        
            ## concept of a subtree is incorrect: when tree2 branches left on a value which is not
            # in the domain in tree1, then must take the right branch in tree2 and stay in the same place in tree1.
            # Need to store assignments as well as domains!!! arg.
            # Or could just navigate down the tree and take the first right branch.
    def subset(self, node2):
        # Are this node's domains subsets of those of node2? 
        # When doing the subset, the node number must be <= as well
        # And are the nodes under the self node a subtree?
        
        # Assume we always branch on the first value in the first non-unit domain.
        
        if not hasattr(node2, "domains"):
            assert hasattr(node2, "solution")
            print "Tree 2 reached a solution before tree 1, therefore the constraint is overpropagating."
            print "This happened below node %d with solution %d" %(self.nodenum, node2.solnum)
            return False
        
        if not subsetdomains(self.domains, node2.domains):
            print "Domains not a subset when comparing nodes %d (in tree 1) and %d (in tree 2)"%(self.nodenum, node2.nodenum)
            return False
        
        if self.nodenum==0:   # special handling for weird node 0
            assert node2.nodenum==0
            return self.left.subset(node2.left)
            
        if self.nodenum>node2.nodenum:
            print "Problem with node numbers with nodes %d and %d"%(self.nodenum, node2.nodenum)
            return False
        
        # work out which value was instantiated for left in tree2
        assert firstunassigned(self.domains)>=firstunassigned(node2.domains)
        var=firstunassigned(node2.domains)
        val1=self.domains[var][0]
        val2=node2.domains[var][0]
        assert val1>=val2
        if val1>val2:
            # need to descend the right branch of node2 while keeping the same position in tree1.
            if hasattr(node2, "left") and node2.left.solutionbelow():
                print "Spurious solution below left branch from node %d in tree 2"%node2.nodenum
                return False
            return self.subset(node2.right)
        
        # now handle the case where the left branch is necessary in tree2 but not
        # tree1 (because the variable is instantiated in tree 1)
        if val1==val2 and len(self.domains[var])==1:
            if hasattr(node2, "right") and node2.right.solutionbelow():
                print "Spurious solution(s) below node %d in tree 2"%node2.nodenum
                return False
            return self.subset(node2.left)
        
        if hasattr(self, "left") and not hasattr(node2, "left"):
            print "Node %d has left child (%d) in smaller tree, but node %d does not in larger tree."%(self.nodenum, self.left.nodenum, node2.nodenum)
            return False
        if hasattr(self, "right") and not hasattr(node2, "right"):
            print "Node %d has right child (%d) in smaller tree, but node %d does not in larger tree."%(self.nodenum, self.right.nodenum, node2.nodenum)
            return False
        if hasattr(self, "left") and not self.left.subset(node2.left):
            return False
        if hasattr(self, "right") and not self.right.subset(node2.right):
            return False
        if hasattr(node2, "left") and not hasattr(self, "left"):
            if node2.left.solutionbelow():
                print "Spurious solution below node %d in tree 2"% node2.left.nodenum
                return False
        if hasattr(node2, "right") and not hasattr(self, "right"):
            if node2.right.solutionbelow():
                print "Spurious solution below node %d in tree 2"% node2.right.nodenum
                return False
        return True

def firstunassigned(l):
    for i in range(1, len(l)+1):
        if len(l[i])>1:
            return i
    return False

def subsetlist(i, j):
    # is i a subset of j
    for e in i:
        if not e in j:
            return False
    return True

def subsetdomains(dom1, dom2):
    assert len(dom1)==len(dom2)
    for i in range(1, len(dom1)+1):
        if not subsetlist(dom1[i], dom2[i]):
            return False
    return True

def setdifference(set1, set2):  #set1/ set2
    diff=[]
    for e in set1:
        if e not in set2:
            diff.append(e)
    return diff

class tree:
    def buildtree(self, filename):
        # use node.left and node.right and node.parent elements to construct a tree of node and sol
        linesraw=open(filename, 'r').readlines()
        tags=["Node", "Sol", "SearchAssign", "SearchAction", "Solution Number"]  # These are the tags to pick out of the file.
        def filt(x):
            (left, temp, right)=partition(":", x)
            return left in tags
        lines=[x for x in linesraw if filt(x)]
        # Now merge adjacent solution lines into one
        i=0
        while i<len(lines)-1:
            (tag, temp, rest)=partition(":", lines[i])
            (tag2, temp, rest2)=partition(":", lines[i+1])
            if tag!="Sol" or tag2!="Sol":
                i=i+1
            else:
                newstring="Sol:"+rest.strip()+" "+rest2.strip()
                lines[i:i+2]=[newstring]
        
        #print "Entering buildtree"
        rootnode=False
        curnode=False
        curassign=False
        for line in lines:
            (tag, temp, rl1)=partition(":", line)
            if rootnode==False:
                assert tag=="Node"
                rootnode=node()
                rootnode.read(rl1)
                curnode=rootnode
            else:
                if tag=="Node":
                    if curassign=="=" or (hasattr(curnode, "nodenum") and curnode.nodenum==0):
                        # only the first node has a child without an assignment happening
                        curnode.left=node()
                        curnode.left.parent=curnode  # weak ref to help with gc.
                        curnode=curnode.left
                        curnode.read(rl1)
                    else:
                        assert curassign=="!="
                        curnode.right=node()
                        curnode.right.parent=curnode
                        curnode=curnode.right
                        curnode.read(rl1)
                    curassign=False
                elif tag=="Sol":
                    if curassign=="=" or curassign==False:
                        curnode.left=sol()
                        curnode.left.parent=curnode
                        curnode=curnode.left
                        curnode.read(rl1)
                    else:
                        curnode.right=sol()
                        curnode.right.parent=curnode
                        curnode=curnode.right
                        curnode.read(rl1)
                    curassign=False
                elif tag=="SearchAction":
                    assert rl1.strip()=="bt"
                    curassign=False
                elif tag=="Solution Number":
                    assert hasattr(curnode, "solution")
                    curnode.solnum=int(rl1.strip())
                else:
                    assert tag=="SearchAssign"
                    assert not curassign
                    (part1, temp, part2)=partition("=", rl1)
                    if part1[-1]=="!":
                        curassign="!="
                        part1=part1[:-1]
                        while hasattr(curnode, "solution") or curnode.assignpair!=(part1, part2):
                            curnode=curnode.parent
                    else:
                        curassign="="
                        curnode.assignpair=(part1, part2)
        self.rootnode=rootnode
    
    def printtree(self, curnode=False):
        if not curnode:
            curnode=self.rootnode
            if not curnode:
                return
        if hasattr(curnode, "nodenum"):
            print curnode.nodenum
        else:
            print curnode.solution
        if hasattr(curnode, "left"):
            print "left branch:"
            self.printtree(curnode.left)
        if hasattr(curnode, "right"):
            print "right branch:"
            self.printtree(curnode.right)
    
    def outputdot(self, fileh):
        fileh.write("digraph name0 {\n")
        self.rootnode.outputdot(fileh)
        fileh.write("}\n")
    
    def equal(self, tree2):
        if self.rootnode==False or tree2.rootnode==False:
            return self.rootnode==tree2.rootnode
        return self.rootnode.equal(tree2.rootnode)
    
    def subset(self, tree2):
        if self.rootnode==False:
            if tree2.rootnode==False:
                return True
            else:
                return not tree2.rootnode.solutionbelow()
        else:
            if tree2.rootnode==False:
                print "No nodes present in tree 2."
                return False
            else:
                return self.rootnode.subset(tree2.rootnode)

################################################################################
#
# Classes for testing each constraint

import random

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
        
    def runtest(self, options=dict()):
        return runtestgeneral("modulo", True, options, [1,1,1], ["posnum", "posnum", "posnum"], [1,1,1], self, False)

class testgacelement__minus__deprecated:
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
    
    def runtest(self, options=dict()):
        return runtestgeneral("gacelement-deprecated", False, options, [4,1,1], ["smallnum", "num", "num"], [4,1,1], self, not options['reify'])

class testwatchelement(testgacelement__minus__deprecated):
    def runtest(self, options=dict()):
        return runtestgeneral("watchelement", False, options, [4,1,1], ["smallnum", "num", "num"], [4,1,1], self, not options['reify'])

class testelement(testgacelement__minus__deprecated):
    def runtest(self, options=dict()):
        return runtestgeneral("element", False, options, [4,1,1], ["smallnum", "num", "num"], [4,1,1], self, False)

class testnegativetable:
    def printtable(self, domains, tab):
        cross=[]
        crossprod(domains,[], cross)
        return setdifference(cross, tab)
    
    def randomtable(self, domains):
        cross=[]
        crossprod(domains,[], cross)
        temp=random.sample(cross, len(cross)/10)
        temp.sort()
        return temp
    
    def runtest(self, options=dict()):
        constraintname="negativetable"
        varnums=[5]
        vartypes=["quitesmallnum"]
        howprintvars=[5]
        tablegen=self
        treesame=True
        
        (domlists, modvars, tablevars)=generatevariables(varnums, vartypes, False)
        
        curvar=0
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
        
        constraint=constraint[:-1]+",modtable)"   # hack off the last comma and put a bracket
        
        toopt=random.randint(0,3)
        optvar=random.randint(0, sum(varnums)-1)
        
        optline=False
        if toopt==0:
            # 1 in 4 chance
            minmax=random.randint(0,1)
            if minmax==1:
                optline="MAXIMIZING x%d"%optvar
            else:
                optline="MINIMIZING x%d"%optvar
        
        negtuplelist=tablegen.randomtable(domlists)
        tuplelist=tablegen.printtable(domlists, negtuplelist)
        
        # now convert tuplelist into a string.
        tuplestring="modtable %d %d \n"%(len(tuplelist), sum(varnums))
        for l in tuplelist:
            for e in l:
                tuplestring+="%d "%e
            tuplestring+="\n"
        
        negtuplestring="modtable %d %d \n"%(len(negtuplelist), sum(varnums))
        for l in negtuplelist:
            for e in l:
                negtuplestring+="%d "%e
            negtuplestring+="\n"
        # tuplelist is actually a set of lists(not yet), so that it can be reformed for reify or reifyimply
        
        constrainttable="table([x0"
        for i in range(1,sum(varnums)):
            constrainttable+=",x%d"%i
        constrainttable+="], modtable)"
        
        # add some other constraints at random into the constraint and constrainttable strings
        if random.randint(0,1)==0:
            for i in range(sum(varnums)-2):
                var1=random.randint(0, sum(varnums)-1)
                var2=random.randint(0, sum(varnums)-1)
                while var1==var2: 
                    var2=random.randint(0, sum(varnums)-1)
                ctype=random.randint(0,2)
                if ctype==0:
                    c="diseq(x%d, x%d)"%(var1, var2)
                elif ctype==1:
                    c="eq(x%d, x%d)"%(var1, var2)
                elif ctype==2:
                    c="ineq(x%d, x%d, 0)"%(var1, var2)
                constraint+="\n"+c
                constrainttable+="\n"+c
        
        retval1=runminion("infile1.minion", "outfile1", tablegen.solver, tablevars, constrainttable, tuplelist=tuplestring, opt=optline, printcmd=options['printcmd'])
        retval2=runminion("infile2.minion", "outfile2", tablegen.solver, modvars, constraint, tuplelist=negtuplestring, opt=optline, printcmd=options['printcmd'])
        if retval1!=0 or retval2!=0:
            print "Minion exit values for infile1.minion, infile2.minion: %d, %d"%(retval1, retval2)
            return False
        return comparetrees(treesame)  # tree subset

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
        
    def runtest(self, options=dict()):
        return runtestgeneral("alldiff", True, options, [5], ["quitesmallnum"], [5], self, False)

class testalldiffgacslow(testalldiff):
    def runtest(self, options=dict()):
        return runtestgeneral("alldiffgacslow", False, options, [5], ["quitesmallnum"], [5], self, not options['reify'])

class testgacalldiff(testalldiff):
    def runtest(self, options=dict()):
        return runtestgeneral("gacalldiff", False, options, [5], ["quitesmallnum"], [5], self, not options['reify'])


class testwatchedalldiff(testalldiff):
    def runtest(self, options=dict()):
        return runtestgeneral("watchedalldiff", False, options, [5], ["quitesmallnum"], [5], self, not options['reify'])

class testdiseq(testalldiff):
    def runtest(self, options=dict()):
        return runtestgeneral("diseq", True, options, [1,1], ["num", "num"], [1,1], self, not options['reify'])

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
        
    def runtest(self, options=dict()):
        return runtestgeneral("eq", True, options, [1,1], ["num", "num"], [1,1], self, False)

class testwatchneq:
    # printtable essentially sets up pairsame constraint. negation of alldiff.
    def printtable(self, domains):
        cross=[]
        out=[]
        crossprod(domains, [], cross)
        for l in cross:
            if l[0] != l[1] :
                out.append(l)
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("watchneq", False, options, [1,1], ["num", "num"], [1,1], self, True)

class testwatchless:
    def printtable(self, domains):
        cross=[]
        out=[]
        crossprod(domains, [], cross)
        for l in cross:
            if l[0] < l[1] :
                out.append(l)
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("watchless", True, options, [1,1], ["num", "num"], [1,1], self, True)
		
class testineq:
    def printtable(self, domains):
        const=self.constants[0]
        #  x1 <= x2+const 
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        numtuples=0
        for l in cross:
            if l[0] <= (l[1]+const):
                out.append(l)
        return out
    
    def runtest(self, options=dict()):
        return runtestgeneral("ineq", True, options, [1,1,1], ["num", "num", "const"], [1,1,1], self, True)

class testabs:
    def printtable(self, domains):
    #  x1 = abs(x2) 
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        numtuples=0
        for l in cross:
            if l[0] == abs(l[1]):
                out.append(l)
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("abs", True, options, [1,1], ["num","num"], [1,1], self, False)

class testhamming:
    def printtable(self, domains):
        const=self.constants[0]
        #  x1 <= x2+const 
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
          l1=l[:len(l)/2]
          l2=l[len(l)/2:]
          if sum([  l1[i] != l2[i] for i in xrange(len(l1))] ) >= const:
            out.append(l)
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("hamming", True, options, [4,4,1], ["smallnum", "smallnum", "const"], [4,4,1], self, True)


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
    
    def runtest(self, options=dict()):
        return runtestgeneral("lexleq", True, options, [4,4], ["smallnum", "smallnum"], [4,4], self, True)

class testlexless(testlexleq):
    def printtable(self, domains):
        return testlexleq.printtable(self, domains, less=True)
    
    def runtest(self, options=dict()):
        return runtestgeneral("lexless", True, options, [4,4], ["smallnum", "smallnum"], [4,4], self, True)

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
    
    def runtest(self, options=dict()):
        return runtestgeneral("max", True, options, [5,1], ["smallnum", "num"], [5,1], self, False)
    
class testmin(testmax):
    def printtable(self, domains):
        return testmax.printtable(self, domains, ismax=False)
    
    def runtest(self, options=dict()):
        return runtestgeneral("min", True, options, [5,1], ["smallnum", "num"], [5,1], self, False)

class testoccurrence:
    def printtable(self, domains, leq=False, geq=False):
        # last var is the occurrence parameter.
        cross=[]
        out=[]
        crossprod(domains, [], cross)
        for l in cross:
            if l[:-1].count(self.constants[0])==l[-1]:
                out.append(l)
            elif leq and l[:-1].count(self.constants[0])<=l[-1]:
                out.append(l)
            elif geq and l[:-1].count(self.constants[0])>=l[-1]:
                out.append(l)
        return out
    
    def runtest(self, options=dict()):
        # note that the constant generated may be completely inappropriate. e.g. some value which is not even in the domains.
        return runtestgeneral("occurrence", False, options, [6, 1, 1], ["smallnum", "smallconst", "num"], [6, 1, 1], self, False)

class testoccurrenceleq:
    def printtable(self, domains, leq=True, geq=False):
        cross=[]
        out=[]
        crossprod(domains, [], cross)
        for l in cross:
            if leq and l.count(self.constants[0])<=self.constants[1]:
                out.append(l)
            elif geq and l.count(self.constants[0])>=self.constants[1]:
                out.append(l)
        return out
    
    def runtest(self, options=dict()):
        # note that the constant generated may be completely inappropriate. e.g. some value which is not even in the domains.
        return runtestgeneral("occurrenceleq", False, options, [6,1,1], ["smallnum", "smallconst", "smallconst"], [6,1,1], self, not options['reify'] and not options['reifyimply'])

class testoccurrencegeq(testoccurrenceleq):
    def printtable(self, domains):
        return testoccurrenceleq.printtable(self, domains, leq=False, geq=True)
    
    def runtest(self, options=dict()):
        # note that the constant generated may be completely inappropriate. e.g. some value which is not even in the domains.
        return runtestgeneral("occurrencegeq", False, options, [6,1,1], ["smallnum", "smallconst", "smallconst"], [6,1,1], self, False)

class testproduct:
    def printtable(self, domains): 
        out=[]
        for i in domains[0]:
            for j in domains[1]:
                if(i*j in domains[2]):
                    out.append([i, j, i*j])
        return out
    
    def runtest(self, options=dict()):
        return runtestgeneral("product", True, options, [1,1,1], ["num", "num", "num"], [1,1,1], self, False)

class testdifference:
    def printtable(self, domains): 
        out=[]
        for i in domains[0]:
            for j in domains[1]:
                if(abs(i-j) in domains[2]):
                    out.append([i, j, abs(i-j)])
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("difference", True, options, [1,1,1], ["num", "num", "num"], [1,1,1], self, False)


class testgacsum:
    def printtable(self, domains):
        cross=[]
        out=[]
        crossprod(domains[:-1], [], cross)
        for i in cross:
            if sum(i) in domains[-1]:
                out.append(i+[sum(i)])
        return out
    
    def runtest(self, options=dict()):
        return runtestgeneral("gacsum", False, options, [3], ["num"], [1,1,1], self, True)

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
    
    def runtest(self, options=dict()):
        return runtestgeneral("sumgeq", True, options, [5,1], ["smallnum", "num"], [5,1], self, False)
    
class testsumleq(testsumgeq):
    def printtable(self, domains):
        return testsumgeq.printtable(self, domains, less=True)
    
    def runtest(self, options=dict()):
        return runtestgeneral("sumleq", True, options, [5,1], ["smallnum", "num"], [5,1], self, False)

class testdiv:
    def printtable(self, domains):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            if l[0]//l[1] == l[2]:
                out.append(l)
        return out
        
    def runtest(self, options=dict()):
        return runtestgeneral("div", True, options, [1,1,1], ["posnum", "posnum", "posnum"], [1,1,1], self, False)

class testweightedsumgeq(testsumgeq):
    def printtable(self, domains):
        return testsumgeq.printtable(self, domains, weights=self.constants)
    
    def runtest(self, options=dict()):
        return runtestgeneral("weightedsumgeq", True, options, [5,5,1], ["const", "smallnum", "num"], [5,5,1], self, False)
        
class testweightedsumleq(testsumgeq):
    def printtable(self, domains):
        return testsumgeq.printtable(self, domains, less=True, weights=self.constants)
    
    def runtest(self, options=dict()):
        return runtestgeneral("weightedsumleq", True, options, [5,5,1], ["const", "smallnum", "num"], [5,5,1], self, False)

class testminuseq:
    def printtable(self, domains):
        out=[]
        for i in domains[0]:
            if (-i) in domains[1]:
                out.append([i, -i])
        return out
    
    def runtest(self, options=dict()):
        return runtestgeneral("minuseq", True, options, [1,1], ["num", "num"], [1,1], self, not options['reifyimply'] and not options['reify'])
        
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
    
    def runtest(self, options=dict()):
        return runtestgeneral("litsumeq", True, options, [4,1], ["smallnum", "num"], [4,1], self, False)

class testwatchsumgeq:
    def printtable(self, domains, less=False):
        const=self.constants[0]
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            sumval=sum(l)
            if (not less and sumval>=const) or (less and sumval<=const):
                out.append(l)
        return out
    
    def runtest(self, options=dict()):
        return runtestgeneral("watchsumgeq", True, options, [5,1], ["boolean", "const"], [5,1], self, True)
    
class testwatchsumleq(testwatchsumgeq):
    def printtable(self, domains):
        return testwatchsumgeq.printtable(self, domains, less=True)
    def runtest(self, options=dict()):
        return runtestgeneral("watchsumleq", True, options, [5,1], ["boolean", "const"], [5,1], self, True)
        
class testwatchvecneq:
    def printtable(self, domains):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            if l[:len(l)/2]!=l[len(l)/2:]:
                out.append(l)
        return out
    
    def runtest(self, options=dict()):
        return runtestgeneral("watchvecneq", True, options, [3,3], ["smallnum","smallnum"], [3,3], self, True)
    
class testwatchvecexists_less:
    def printtable(self, domains):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            if sum([(l[i]< l[len(l)/2+i]) for i in xrange(len(l)/2)])>0:
                out.append(l)
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("watchvecexists_less", True, options, [3,3], ["smallnum", "smallnum"], [3,3], self, True)

class testwatchvecexists_and:
    def printtable(self, domains):
        cross=[]
        crossprod(domains, [], cross)
        out=[]
        for l in cross:
            if sum([(l[i]>0 and l[len(l)/2+i]>0) for i in xrange(len(l)/2)] )>0:
                out.append(l)
        return out

    def runtest(self, options=dict()):
        return runtestgeneral("watchvecexists_and", True, options, [3,3], ["smallnum", "smallnum"], [3,3], self, True)
                        
class testpow:
    def printtable(self, domains):
        cross=[]
        crossprod(domains[:-1], [], cross)
        out=[]
        for l in cross:
            if l[0]**l[1] in domains[-1]:
                out.append(l+[l[0]**l[1]])
        return out
    def runtest(self, options=dict()):
        return runtestgeneral("pow", True, options, [1,1,1], ["posnum","posnum","posnum"], [1,1,1], self, False)
    
class testgcc:
    def printtable(self, domains):
        vals=self.constants
        cross=[]
        x=domains[:len(domains)/2]
        cap=domains[len(domains)/2:]
        crossprod(x, [], cross)
        # assume same number of x vars and cap vars.
        dom_min=min([min(y) for y in x])
        out=[]
        for line in cross:
            occ=[0 for y in vals]
            for xi in line:
                for i in range(len(vals)):
                    if vals[i]==xi:
                        occ[i]+=1
            flag=True
            for i in range(len(occ)):
                if occ[i] not in cap[i]:
                    flag=False
            if flag:
                out.append(line+occ)
        return out
        
    def runtest(self, options=dict()):
        if options['reifyimply'] or options['reify']:
            return runtestgeneral("gcc", False, options, [4,4,4], ["verysmallnum","smallconst","smallnum"], [4,4,4], self, False)
        else:
            return runtestgeneral("gcc", False, options, [5,5,5], ["smallnum","smallconst", "num"], [5,5,5], self, False)
    
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
def printminionfile(fileh, variables, constraint, tuplelist=False, opt=False):
    # constraint parameter must include reify if required.
    fileh.write("MINION 3\n")
    fileh.write("\n**VARIABLES**\n")
    fileh.write(variables)
    if tuplelist:
        fileh.write("\n**TUPLELIST**\n")
        fileh.write(tuplelist)
    if opt:
        fileh.write("\n**SEARCH**\n")
        fileh.write(opt)
    fileh.write("\n**CONSTRAINTS**\n")
    fileh.write(constraint)
    fileh.write("\n**EOF**")

def runtestgeneral(constraintname, boundsallowed, options, varnums, vartypes, howprintvars, tablegen, treesame):
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
    # fullprop compares the constraint against its full-propagated version.
    reifyimply=options['reifyimply']
    reify=options['reify']
    fullprop=options['fullprop']
    
    if reify or reifyimply:
        # add extra bool variable.
        varnums=[1]+varnums
        vartypes=["boolean"]+vartypes
    
    (domlists, modvars, tablevars, constants)=generatevariables(varnums, vartypes, boundsallowed)
    setattr(tablegen, "constants", constants)
    
    if modvars.find("BOUND")!=-1:
        treesame=False    # Assume can't have GAC when there are bound variables around.
    
    curvar=0
    varnums3=varnums
    vartypes3=vartypes
    if reify or reifyimply:
        curvar=1
        varnums3=varnums[1:]
        vartypes3=vartypes[1:]
    
    constraint=constraintname+"("
    
    constnum=0   # number of the current constant
    
    for (num,typ) in zip(varnums3, vartypes3):
        if typ=="const" or typ=="smallconst":
            if num>1:
                # print vector of constants
                constraint+="[%d"%constants[constnum]
                constnum+=1
                for e in range(num-1):
                    constraint+=", %d"%constants[constnum]
                    constnum+=1
                constraint+="],"
            else:
                # print single constant
                constraint+="%d,"%constants[constnum]
                constnum+=1
        elif num>1:
            #print vector
            constraint+="[x%d"%curvar
            curvar+=1
            for e in range(num-1):
                constraint+=", x%d"%curvar
                curvar+=1
            constraint+="],"
        else:
            assert num==1
            #print single variable.
            constraint+="x%d,"%curvar
            curvar+=1
    
    constraint=constraint[:-1]+")"   # hack off the last comma and put a bracket
    
    if reify:
        constraint="reify("+constraint+", x0)"
    if reifyimply:
        constraint="reifyimply("+constraint+", x0)"
    
    toopt=random.randint(0,3)
    
    varnums2=varnums[:]
    for (i,t) in zip(range(len(varnums)), vartypes):
        if t in ["const", "smallconst"]:
            varnums2[i]=0   # constants, so don't count as vars.
    optvar=random.randint(0, sum(varnums2)-1)
    
    optline=False
    if toopt==0:
        # 1 in 4 chance
        minmax=random.randint(0,1)
        if minmax==1:
            optline="MAXIMIZING x%d"%optvar
        else:
            optline="MINIMIZING x%d"%optvar
    
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
    tuplestring="modtable %d %d \n"%(len(tuplelist), sum(varnums2))
    for l in tuplelist:
        for e in l:
            tuplestring+="%d "%e
        tuplestring+="\n"
    
    # tuplelist is actually a set of lists(not yet), so that it can be reformed for reify or reifyimply
    
    constrainttable="table([x0"
    for i in range(1,sum(varnums2)):
        constrainttable+=",x%d"%i
    constrainttable+="], modtable)"
    
    # add some other constraints at random into the constraint and constrainttable strings
    if random.randint(0,1)==0:
        for i in range(sum(varnums2)-2):
            var1=random.randint(0, sum(varnums2)-1)
            var2=random.randint(0, sum(varnums2)-1)
            while var1==var2: 
                var2=random.randint(0, sum(varnums2)-1)
            ctype=random.randint(0,2)
            if ctype==0:
                c="diseq(x%d, x%d)"%(var1, var2)
            elif ctype==1:
                c="eq(x%d, x%d)"%(var1, var2)
            elif ctype==2:
                c="ineq(x%d, x%d, 0)"%(var1, var2)
            constraint+="\n"+c
            constrainttable+="\n"+c
    
    if not fullprop:
        retval1=runminion(str(os.getpid())+"infile1.minion", str(os.getpid())+"outfile1", tablegen.solver, tablevars, constrainttable, tuplelist=tuplestring, opt=optline, printcmd=options['printcmd'])
        retval2=runminion(str(os.getpid())+"infile2.minion", str(os.getpid())+"outfile2", tablegen.solver, modvars, constraint, opt=optline, printcmd=options['printcmd'])
        if retval1!=0 or retval2!=0:
            print "Minion exit values for infile1.minion, infile2.minion: %d, %d"%(retval1, retval2)
            return False
        if boundsallowed and modvars.rfind("SPARSEBOUND")!=-1:   
            # This is a temporary hack while sparsebound does not print its domain properly.
            treesame=False
        return comparetrees(treesame)  # tree subset
    else:
        retval1=runminion(str(os.getpid())+"infile1.minion", str(os.getpid())+"outfile1", tablegen.solver, modvars, constraint, opt=optline, printcmd=options['printcmd'], cmd="-fullprop")
        retval2=runminion(str(os.getpid())+"infile2.minion", str(os.getpid())+"outfile2", tablegen.solver, modvars, constraint, opt=optline, printcmd=options['printcmd'])
        if retval1!=0 or retval2!=0:
            print "Minion exit values for infile1.minion, infile2.minion: %d, %d"%(retval1, retval2)
            return False
        if boundsallowed and modvars.rfind("SPARSEBOUND")!=-1:   
            # This is a temporary hack while sparsebound does not print its domain properly.
            treesame=False
        return comparetrees(True)  # trees same.
    
def runminion(filename, outfilename, minionbin, variables, constraint, tuplelist=False, opt=False, printcmd=False, cmd=""):
    file1=open(filename, "w")
    printminionfile(file1, variables, constraint, tuplelist=tuplelist, opt=opt)
    file1.close()
    cmd=minionbin+" -dumptree -findallsols "+cmd+" "+filename+" >"+outfilename
    if printcmd:
        print "Executing command: "+cmd
    return os.system(cmd)

def generatevariables(varblocks, types, boundallowed):
    # generate a set of variables with random domains and types
    # Varblocks specifies the groups which should be of the same type
    # sum(varblocks) is the total number of variables/constants.
    # types specifies num, posnum, nonnegnum or boolean for each block
    # boundallowed specifies if it is appropriate to have bound variables.
    st_nontable=""
    st_table=""
    domainlists=[]
    constants=[]
    typesinczero=["num", "smallnum", "quitesmallnum",  "nonnegnum", "boolean"]
    typesconst=["const", "smallconst"]
    varblocks2=varblocks[:]
    for (i,t) in zip(range(len(varblocks)), types):
        if t in typesconst:
            varblocks2[i]=0   # constants, so don't count as vars.
    
    for i in range(len(varblocks)):
        # randomly choose the type of the variables
        if types[i] not in typesconst:
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
        else:
            ty=None
        
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
            elif types[i]=="veryverysmallnum":
                lb=random.randint(-1, 0)   # max 3 vals in domain.
                ub=random.randint(0, 1)
            elif types[i]=="posnum":
                lb=random.randint(1, 20)
                ub=random.randint(21, 40)
            elif types[i]=="nonnegnum":
                lb=random.randint(0, 20)
                ub=random.randint(21, 40)
            elif types[i]=="boolean":
                lb=0
                ub=1
            elif types[i]=="const":
                lb=random.randint(-20, 20)
                ub=lb
            elif types[i]=="smallconst":
                lb=random.randint(-5, 5)
                ub=lb
            else:
                assert False
            
            varnum=sum(varblocks2[0:i])+j
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
            elif ty=="DISCRETE ":
                st_nontable+=ty+"x%d {%d..%d}\n"%(varnum, lb, ub)
                st_table   +=ty+"x%d {%d..%d}\n"%(varnum, lb, ub)
                domainlists.append(range(lb, ub+1))
            else:
                # a constant type. Do not append to domainlists
                # but return in a constants list
                constants.append(lb)
                
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
    return (domainlists, st_nontable, st_table, constants)

def comparetrees(same):
    # Fork so that memory used in building and comparing
    # trees in the child process is returned when the process ends.
    r, w = os.pipe() # these are file descriptors, not file objects
    
    parentpid=os.getpid()

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
        
        tree1.buildtree(str(parentpid)+"outfile1")
        tree2.buildtree(str(parentpid)+"outfile2")
        
        if same:
            temp= tree1.equal(tree2)
        else:
            temp= tree1.subset(tree2)
        
        # now return 
        if temp:
            os.remove(str(parentpid)+"outfile1")
            os.remove(str(parentpid)+"outfile2")
            os.remove(str(parentpid)+"infile1.minion")
            os.remove(str(parentpid)+"infile2.minion")
            w.write("True")
        
        w.close()
        sys.exit(0)


