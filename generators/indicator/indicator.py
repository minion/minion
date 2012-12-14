#!/usr/bin/python

from itertools import *

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
    print("table([" + ', '.join([print_var(a, domainsize) for a in varlist]) + "], " + name + ")")



def generate_constraint(CurrentTupleList, ConstraintTupleList):
   returnList = []
   for i in CurrentTupleList:
       for j in ConstraintTupleList:
        returnList.append(i + [j])
   return returnList

def generate_indicator(ConstraintList, domainsize, indicatorsize, check_idempotent = True):
  print("MINION 3")
  print("**VARIABLES**")
  print("DISCRETE V[%d] {0..%d}" % ( domainsize ** indicatorsize, domainsize - 1))
  print("**TUPLELIST**")
  
  for i in range(len(ConstraintList)):
    print("Table%d %d %d" % (i, len(ConstraintList[i]), len(ConstraintList[i][0])))
    for tuple in ConstraintList[i]:
      print(" ".join(["%d" % t for t in tuple]))
  
  print("**CONSTRAINTS**")

  for i in range(len(ConstraintList)):
      Constraint = ConstraintList[i]

      PositionList = [ [] ]
      for j in range(indicatorsize):
          PositionList = generate_constraint(PositionList, Constraint)
      for p in PositionList:
        print_constraint(p, "Table%d" % i, domainsize)
  if check_idempotent:
    for i in range(domainsize):
      print("eq(V[%d], %d)" % (list_to_int([i]*indicatorsize, domainsize), i))
  print("**EOF**")

def build_table(X, domain_size, arity):
  return filter(X, product(range(domain_size), repeat = arity))

neqtable = lambda (x,y): x != y
diff1    = lambda (x,y): abs(x-y) > 1
xlessy   = lambda (x,y): x < y
eqoreq = lambda(a,b,c,d) : (a == b) or (c == d)
eqorneq = lambda(a,b,c,d) : (a == b) or (c != d)

domsize = 2

#generate_indicator([build_table(diff1, 5), build_table(xlessy, 5)], 5, 5)
generate_indicator([build_table(eqorneq,domsize,4), build_table(eqorneq,domsize,4)], domsize,5, False)
