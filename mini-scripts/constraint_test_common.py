import sys
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
        fileh.write("%d label=node%d;\n"%(self.nodenum, self.nodenum) )
        thisname=str(self.nodenum)
        
        if hasattr(self, "left"):
            if hasattr(self.left, "nodenum"):
                leftname=str(self.left.nodenum)
            else:
                leftname="s"+str(self.left.solnum)
            fileh.write("%s -> %s;\n"% (thisname, leftname))
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
        
        print "Entering buildtree"
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

