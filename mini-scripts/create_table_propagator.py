#!/usr/bin/python

# take a list of unsatisfying tuples for a constraint, and create a tree listing
# the propagations that need to be done for that constraint.

# At each node in the tree, branch yes/no on the literal that splits the remaining
# set of unsat tuples most evenly.



# domains_in is the values that are certainly in the domains. 
# domains_poss is the values that are possibly in. i.e. haven't been tested yet.
import copy
import sys
import cProfile

def gac_prunings(nogoods, domains):
    # take a domain list, and a set of nogoods, and calculate the prunings GAC 
    # would do.
    #print "nogoods:"+str(nogoods)
    #print "domains:"+str(domains)
    
    prunings=[]
    for var in xrange(len(domains)):
        for val in domains[var]:
            domains2=domains[:]
            domains2[var]=[val]
            nogoods2=[]
            for t in nogoods:
                flag=True
                for i in xrange(len(domains)):
                    if t[i] not in domains2[i]:
                        flag=False
                        break
                if flag:
                    nogoods2.append(t)
            tuplecount=reduce(lambda x,y:x*y, map(lambda z:len(z), domains2))
            if len(nogoods2) == tuplecount:
                prunings.append((var, val))
    
    #print "prunings:"+str(prunings)
    return prunings


def gac_prunings2(nogoods, domains):
    prunings=[]
    for var in xrange(len(domains)):
        for val in domains[var]:
            tup=[  ]
            
    return []

def increment_tuple(tup, domains, var, val):
    return

checkties=False

checktreecutoff=False

def build_tree(ct_init, tree, domains_in, domains_poss, domains_out, varvalorder, heuristic):
    # take a list of unsatisfying tuples within domains_poss & domains_in.
    # a tree node
    # a domain list
    # and make two new tree nodes ... calls itself recursively.
    # Returns false if it and all its children do not do any pruning.
    #print "Top of build_tree"
    
    # Process the domains -- last value remaining must be 'in'
    # IF domains_poss[var] has only one value and domains_in[var] is empty,
    # then we don't need to do a test -- can assume that the poss value is IN,
    # because otherwise, the domain will be empty and no prop is reqd.
    
    # If only one possible value left, assume it is 'in', otherwise we would have failed already.
    for var in xrange(len(domains_poss)):
        if len(domains_in[var])==0 and len(domains_poss[var])==1:
            domains_in[var]=domains_poss[var]
            domains_poss[var]=[]
    
    # Filter the tuple list -- remove any tuples that are not valid
    # nogoods that are outside the current domains are no longer useful.
    ct=[]
    for t in ct_init:
        flag=True
        for i in xrange(len(t)):
            if (t[i] not in domains_poss[i]) and (t[i] not in domains_in[i]):
                flag=False
                break
        if flag:
            ct.append(t)
    
    if ct == []:
        # The constraint is implied.
        return False  # no pruning.
    
    # find the GAC prunings required at this node.
    whole_domain=[]
    for i in xrange(len(domains_in)):
        whole_domain.append(domains_in[i]+domains_poss[i])
        whole_domain[i].sort()
    
    #print "About to do GAC."
    #print "tuples:"+str(ct)
    #print "domains_poss:"+str(domains_poss)
    #print "domains_in:"+str(domains_in)
    #print "domains:"+str(whole_domain)
    #print "domains_out:"+str(domains_out)
    
    prun=gac_prunings(ct, whole_domain)
    
    #print prun
    
    #print "prun:"+str(prun[:])
    if len(prun)>0:
        tree['pruning']=prun[:]
    
    # Remove the pruned values from the active domains
    for (var,val) in prun:
        if val in domains_in[var]:
            domains_in[var].remove(val)
        else:
            assert val in domains_poss[var]
            domains_poss[var].remove(val)
    
    # check if a domain is empty
    for var in xrange(len(domains_in)):
        if len(domains_in[var])+len(domains_poss[var])==0:
            return True # we did do pruning here.
            # Could change this node from some pruning to a fail.
    
    # If we have complete domain knowledge, then return.
    if len(filter(lambda x:len(x)>0, domains_poss))==0:
        if len(prun)==0:
            return False
        else:
            return True
    
    # Filter the tuple list again after pruning -- remove any tuples that are not valid
    # nogoods that are outside the current domains are no longer useful.
    ct2=[]
    for t in ct:
        flag=True
        for i in xrange(len(t)):
            if (t[i] not in domains_poss[i]) and (t[i] not in domains_in[i]):
                flag=False
                break
        if flag:
            ct2.append(t)
    
    if ct2 == []:
        # The constraint is implied.
        assert len(prun)>0  # Should only reach here if tuples lost by pruning.
        return True
    
    ########################################################################
    #
    #  Branching
    
    chosenvar=-1
    chosenval=0
    
    # default ordering
    if not heuristic:
        for (var, val) in varvalorder:
            if val in domains_poss[var]:
                chosenvar=var
                chosenval=val
                break
    
    # choose the var and val contained in the most remaining nogoods
    # I.e. if it's not in domain, it will eliminate the most nogoods, pushing towards impliedness.
    if heuristic:
        numnogoods=-1
        for (var, val) in varvalorder:
            if val in domains_poss[var]:
                count=len(filter(lambda a: a[var]==val,  ct2))
                if count>numnogoods:
                    numnogoods=count
                    chosenvar=var
                    chosenval=val
        
        if checkties:
            for (var, val) in varvalorder:
                if val in domains_poss[var]:
                    count=len(filter(lambda a: a[var]==val,  ct2))
                    if count==numnogoods:
                        print "Ties"
                        break
    
    if chosenvar==-1:
        # this case arises when the varvalorder does not contain all var val pairs/
        # This might be because the last variable is functionally dependent on the 
        # others.
        # Treated same as complete domain knowledge.
        assert False
        if len(prun)==0:
            return False
        else:
            return True
    
    #print "Chosen variable: %d" %chosenvar
    #print "Chosen value:%d"%chosenval
    
    dom_left_poss=copy.deepcopy(domains_poss)
    dom_right_poss=copy.deepcopy(domains_poss)
    
    dom_left_in=copy.deepcopy(domains_in)
    dom_right_in=copy.deepcopy(domains_in)
    
    dom_right_out=list(domains_out)
    dom_right_out[chosenvar]=dom_right_out[chosenvar]|frozenset([chosenval])
    dom_right_out=tuple(dom_right_out)
    
    # In left tree, move the value from possible to in
    dom_left_in[chosenvar].append(chosenval)
    dom_left_poss[chosenvar].remove(chosenval)
    
    # In right tree, remove the value from possible.
    dom_right_poss[chosenvar].remove(chosenval)
    
    tree['var']=chosenvar
    tree['val']=chosenval
    prun_left=False
    prun_right=False
    
    tree['left']=dict()
    prun_left=build_tree(copy.deepcopy(ct2), tree['left'], dom_left_in, dom_left_poss, domains_out, varvalorder, heuristic)
    if not prun_left:
        if checktreecutoff:
            print "deleting subtree of size: %d"%(tree_cost2(tree['left']))
        del tree['left']
    
    # If we have not emptied the domain in the right branch:
    if len(dom_right_poss[chosenvar])+len(dom_right_in[chosenvar])>0:
        tree['right']=dict()
        prun_right=build_tree(copy.deepcopy(ct2), tree['right'], dom_right_in, dom_right_poss, dom_right_out, varvalorder, heuristic)
        if not prun_right:
            if checktreecutoff:
                print "deleting subtree of size: %d"%(tree_cost2(tree['right']))
            del tree['right']
    
    if (not prun_left) and (not prun_right) and (not tree.has_key('pruning')):
        return False
    else:
        return True


def print_tree(tree, indent="    "):
    if tree.has_key('pruning'):
        for p in tree['pruning']:
            print indent+"var_array[%d].removeFromDomain(%d);"%(p[0], p[1])
    if tree.has_key('left'):
        print indent+"if(var_array[%d].inDomain(%d))"%(tree['var'], tree['val'])
        print indent+"{"
        print_tree(tree['left'], indent+"    ")
        print indent+"}"
        if tree.has_key('right'):
            print indent+"else"
            print indent+"{"
            print_tree(tree['right'], indent+"    ")
            print indent+"}"
    else:
        if tree.has_key('right'):
            print indent+"if(!var_array[%d].inDomain(%d))"%(tree['var'], tree['val'])
            print indent+"{"
            print_tree(tree['right'], indent+"    ")
            print indent+"}"
    return
    
def tree_cost(tree):
    # measure the max depth for now.
    l=0
    r=0
    if tree.has_key('left'):
        l=tree_cost(tree['left'])
    
    if tree.has_key('right'):
        r=tree_cost(tree['right'])
    
    return max(l,r)+1

def tree_cost2(tree):
    # number of nodes.
    l=0
    r=0
    if tree.has_key('left'):
        l=tree_cost2(tree['left'])
    
    if tree.has_key('right'):
        r=tree_cost2(tree['right'])
    
    return l+r+1

def gen_all_perms(permlist, perm, objects):
    if len(objects)==0:
        permlist.append(perm)
        return
    else:
        for i in xrange(len(objects)):
            # append an object onto perm
            perm2=perm[:]
            perm2.append(objects[i])
            objects2=objects[:i]+objects[i+1:]
            gen_all_perms(permlist, perm2, objects2)

def generate_tree(ct_nogoods, domains_init, heuristic):
    bestcost=1000000000
    besttree=[]
    
    permlist=[]
    
    varvals=[(a,b) for a in range(len(domains_init)) for b in domains_init[a] ]
    if heuristic:
        permlist.append(varvals)
    else:
        gen_all_perms(permlist, [], varvals)
    
    for perm in permlist:
        tree=dict()
        domains_in=[ [] for i in domains_init]
        domains_out=tuple([ frozenset() for i in domains_init])
        domains=copy.deepcopy(domains_init)
        build_tree(copy.deepcopy(ct_nogoods), tree, domains_in, domains, domains_out, perm, len(permlist)==1)   # last arg is whether to use heuristic.
        cost=tree_cost2(tree)
        if cost<bestcost:
            bestcost=cost
            besttree=tree
            print "Better tree found, of size:%d"%bestcost
    
    return besttree

def and_constraint():
    # A /\ B = C
    ct_nogoods=[[0,0,1], [0,1,1], [1,0,1], [1,1,0]]
    varvalorder=[(0,0), (0,1), (1,0), (1,1), (2,0), (2,1)]
    domains_init=[[0,1],[0,1],[0,1]]
    
    generate_tree(ct_nogoods, domains_init, False)

def sports_constraint():
    # Channelling constraint from sports scheduling 10
    # Two variables domain 1..10
    # One variable domain 1..45
    # First generate the positive table
    postable=[]
    counter=1
    for i in xrange(1, 11):
        for j in xrange(i+1,11):
            postable.append([i,j,counter])
            counter=counter+1
    
    ct_nogoods=[]
    
    for i in xrange(1, 11):
        for j in xrange(1,11):
            for k in xrange(1, 46):
                t=[i,j,k]
                if t not in postable:
                    ct_nogoods.append(t)
    
    domains_init=[ range(1,11), range(1,11), range(1,46)]
    
    t=generate_tree(ct_nogoods, domains_init, True)
    print_tree(t)

def sumgeqthree():
    # sumgeq-3 on 5 bool vars.
    nogoods=[]
    for a in range(2):
        for b in range(2):
            for c in range(2):
                for d in range(2):
                    for e in range(2):
                        if sum([a,b,c,d,e])<3:
                            nogoods.append([a,b,c,d,e])
    domains_init=[[0,1],[0,1],[0,1], [0,1], [0,1]]
    
    generate_tree(nogoods, domains_init, False)

def pegsol():
    # constraint for peg solitaire.
    nogoods=[]
    
    # One combination satisfies the conjunction.
    nogoods.append([1,0,1,0,0,1,0])
    
    for a in range(2):
        for b in range(2):
            for c in range(2):
                for d in range(2):
                    for e in range(2):
                        for f in range(2):
                            if not (a==1 and b==0 and c==1 and d==0 and e==0 and f==1):
                                # not satisfies the conjunction
                                nogoods.append([a,b,c,d,e,f,1])
    
    domains_init=[[0,1],[0,1],[0,1], [0,1], [0,1], [0,1], [0,1]]
    t=generate_tree(nogoods, domains_init, True)
    print_tree(t)
    print "Depth: "+str(tree_cost(t))
    print "Number of nodes: "+str(tree_cost2(t))
    
def sokoban():
    # x+y=z where y has values -n, -1, 1, n
    nogoods=[]
    varvalorder=[]
    n=2   # width/height of grid.
    
    for x in range(n*n):
        for y in [-n, -1, 1, n]:
            for z in range(n*n):
                if x+y != z:
                    nogoods.append([x,y,z])
    
    domains_init=[range(n*n),[-n, -1, 1, n], range(n*n)]
    
    t=generate_tree(nogoods, domains_init, True)
    print_tree(t)
    print "Depth: "+str(tree_cost(t))
    print "Number of nodes: "+str(tree_cost2(t))
    

# A tree node is a dictionary containing 'var': 0,1,2.... 'val', 'left', 'right', 'pruning'


# get rid of treenodes when there are no nogoods left.

#cProfile.run('sports_constraint()')

#and_constraint()
sokoban()

#sports_constraint()


