#!/usr/bin/python

from itertools import *

import sys, os, getopt

def increment_list(list, domainsize):
  for i in range(len(list)):
    list[i] += 1
    if list[i] != domainsize:
      return False
    list[i] = 0
  return True

def list_to_int(list, domainsize):
  i = 0
  for l in list:
    i = i * domainsize + l
  return i

# Should take as many tuples as arity of cons.
# More efficient method: instead of testing each pair Tuple1, Tuple2,
# generate the pairs that will work. i.e. generate all subsets of Constraint
# of the right size, and which correspond to two all-different tuples.

def print_var(tuple, domainsize):
  return "V[%d]" % list_to_int(tuple, domainsize)

def print_constraint(TupleList, name, domainsize) :
#    print TupleList
    varlist = zip(*TupleList)
    if gentable:
        print "table([" + ', '.join([print_var(a, domainsize) for a in varlist]) + "], " + name + ")"
    elif genlighttable:
        print "lighttable([" + ', '.join([print_var(a, domainsize) for a in varlist]) + "], " + name + ")"
    elif gentest:
        print "test([" + ', '.join([print_var(a, domainsize) for a in varlist]) + "])"
    elif gendecomp:
        print "Don;t be ksilly"



def generate_constraint(CurrentTupleList, ConstraintTupleList):
   returnList = []
   for i in CurrentTupleList:
       for j in ConstraintTupleList:
        returnList.append(i + [j])
   return returnList

def generate_indicator(ConstraintList, domainsize, indicatorsize, check_idempotent = True):
  print "MINION 3"
  print "**VARIABLES**"
  print "DISCRETE V[%d] {0..%d}" % ( domainsize ** indicatorsize, domainsize - 1)
  print "**TUPLELIST**"
  
  for i in range(len(ConstraintList)):
    print "Table%d %d %d" % (i, len(ConstraintList[i]), len(ConstraintList[i][0]))
    for tuple in ConstraintList[i]:
      print " ".join(["%d" % t for t in tuple])
  
  print "**CONSTRAINTS**"

  for i in range(len(ConstraintList)):
      Constraint = ConstraintList[i]

      PositionList = [ [] ]
      for j in range(indicatorsize):
          PositionList = generate_constraint(PositionList, Constraint)
      for p in PositionList:
        print_constraint(p, "Table%d" % i, domainsize)
  if check_idempotent:
    for i in range(domainsize):
      print "eq(V[%d], %d)" % (list_to_int([i]*indicatorsize, domainsize), i)
  print "**EOF**"

def build_table(X, domain_size, arity):
  return filter(X, product(range(domain_size), repeat = arity))

neqtable = lambda (x,y): x != y
diff1    = lambda (x,y): abs(x-y) > 1
xlessy   = lambda (x,y): x < y
eqoreq = lambda(a,b,c,d) : (a == b) or (c == d)
eqorneq = lambda(a,b,c,d) : (a == b) or (c != d)
sumgeqthree= lambda(a,b,c,d,e): (a+b+c+d+e)>=3
alldiff3 = lambda(a,b,c): (a!=b) and (a!=c) and (b!=c)
lexleq = lambda(a,b,c,x,y,z) : [a,b,c] <= [x,y,z]
lexless = lambda(a,b,c,x,y,z,w) : (a+b+c+x+y+z+w) == 0 or (a+b+c+x+y+z+w) == 7
twotup = lambda(a,b,c,d,e,f,g,h,i,j) : [a,b,c,d,e,f,g,h,i,j] in [[0,0,1,0,1,1,1,0,1,0],[1,0,1,1,0,0,0,1,0,1]]

domsize = -1

# check for flags


(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["domsize=", "table", "test", "decomp", "lighttable", "random", "arity="])


# what type of constraint to print out.
gentable=False
gentest=False
gendecomp=False
genlighttable=False

arity=-1
randomct=False

for i in optargs:
    (a1, a2)=i
    if a1=="--table":
        gentable=True
    if a1=="--lighttable":
        genlighttable=True
    elif a1=="--test":
        gentest=True
    elif a1=="--decomp":
        gendecomp=True
    elif a1=="--random":
        randomct=True
    elif a1=="--arity":
        arity=int(a2)
    elif a1=="--domsize":
        domsize=int(a2)

if randomct:
    print "not yet."
else:
    #generate_indicator([build_table(diff1, 5), build_table(xlessy, 5)], 5, 5)
    generate_indicator([build_table(twotup,domsize,arity)], domsize,5, True)

