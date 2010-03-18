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

def build_tree(ct_init, tree, domains_in, domains_poss, domains_out, varvalorder, memotable):
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
    
    # Check the memoization table
    #if memotable.has_key(domains_out):
    #    return False
    
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
        memotable[domains_out]=False
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
            memotable[domains_out]=False
            return False
        else:
            return True
    
    ########################################################################
    #
    #  Branching
    
    chosenvar=-1
    chosenval=0
    
    for (var, val) in varvalorder:
        if val in domains_poss[var]:
            chosenvar=var
            chosenval=val
            break
    
    # very simple way of choosing var val to start with.
    #for var in xrange(len(domains_poss)):
    #    if len(domains_poss[var])>0:
    #        chosenvar=var
    #        chosenval=domains_poss[var][0]
    #        break
    
    
    assert chosenvar!=-1
    
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
    prun_left=build_tree(copy.deepcopy(ct), tree['left'], dom_left_in, dom_left_poss, domains_out, varvalorder, memotable)
    if not prun_left:
        del tree['left']
    
    # If we have not emptied the domain in the right branch:
    if len(dom_right_poss[chosenvar])+len(dom_right_in[chosenvar])>0:
        tree['right']=dict()
        prun_right=build_tree(copy.deepcopy(ct), tree['right'], dom_right_in, dom_right_poss, dom_right_out, varvalorder, memotable)
        if not prun_right:
            del tree['right']
    
    if (not prun_left) and (not prun_right) and (not tree.has_key('pruning')):
        memotable[domains_out]=False
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
    if tree.has_key['left']:
        l=tree_cost(tree['left'])
    
    if tree.has_key['right']:
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

def generate_tree(ct_nogoods, permlist, domains_init):
    bestcost=1000000000
    besttree=[]
    
    for perm in permlist:
        tree=dict()
        domains_in=[ [] for i in domains_init]
        domains_out=tuple([ frozenset() for i in domains_init])
        domains=copy.deepcopy(domains_init)
        memotable=dict()
        #print "About to build tree, perm: "+str(perm)
        build_tree(copy.deepcopy(ct_nogoods), tree, domains_in, domains, domains_out, perm, memotable)
        cost=tree_cost2(tree)
        if cost<bestcost:
            bestcost=cost
            besttree=tree
            print "Better tree found:"
            print_tree(tree)
    
    print "// Best tree:"
    print_tree(besttree)

def and_constraint():
    # A /\ B = C
    ct_nogoods=[[0,0,1], [0,1,1], [1,0,1], [1,1,0]]
    varvalorder=[(0,0), (0,1), (1,0), (1,1), (2,0), (2,1)]
    domains_init=[[0,1],[0,1],[0,1]]
    permlist=[]
    gen_all_perms(permlist, [], varvalorder)
    #permlist=[[(0, 1), (0, 0), (1, 0), (1, 1), (2, 0), (2, 1)]]
    generate_tree(ct_nogoods, permlist, domains_init)

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
    
    domains_init=[ range(1,2), range(1,11), range(1,46)]
    varvalorder=[ (0,j) for j in xrange(1,11) ]+[ (1,j) for j in xrange(1,11) ]+[ (2,j) for j in xrange(1,46) ]
    permlist=[varvalorder]
    generate_tree(ct_nogoods, permlist, domains_init)

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
    
    varvalorder=[(a,b) for a in range(5) for b in range(2) ]
    
    domains_init=[[0,1],[0,1],[0,1], [0,1], [0,1]]
    permlist=[]
    gen_all_perms(permlist, [], varvalorder)
    
    generate_tree(nogoods, permlist, domains_init)

# A tree node is a dictionary containing 'var': 0,1,2.... 'val', 'left', 'right', 'pruning'


# get rid of treenodes when there are no nogoods left.

#cProfile.run('sports_constraint()')

#and_constraint()
sumgeqthree()


