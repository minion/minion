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

def possibly_generate_constraint(Tuple1, Tuple2, Constraint, name, domainsize) :
  for i in range(len(Tuple1)):
    if not((Tuple1[i], Tuple2[i]) in Constraint):
      return
  print "table([V[%d], V[%d]], %s)" % (list_to_int(Tuple1, domainsize), list_to_int(Tuple2, domainsize), name)


def generate_indicator(ConstraintList, domainsize, indicatorsize, check_idempotent = True):
  print "MINION 3"
  print "**VARIABLES**"
  print "DISCRETE V[%d] {0..%d}" % ( domainsize ** indicatorsize, domainsize - 1)
  print "**TUPLELIST**"
  
  for i in range(len(ConstraintList)):
    print "Table%d %d %d" % (i, len(ConstraintList[i]), len(ConstraintList[i][0]))
    for tuple in ConstraintList[i]:
      print reduce(lambda x,y:str(x)+" "+str(y), tuple)
  
  print "**CONSTRAINTS**"
  for i in range(len(ConstraintList)):
      Constraint = ConstraintList[i]
      Tuple1 = [0] * indicatorsize
      finished1 = False
      while not finished1:
	Tuple2 = [0] * indicatorsize
	finished2 = False
	while not finished2:
	  possibly_generate_constraint(Tuple1, Tuple2, Constraint, "Table" + str(i), domainsize)
	  finished2 = increment_list(Tuple2, domainsize)
	finished1 = increment_list(Tuple1, domainsize)
  if check_idempotent:
    for i in range(domainsize):
      print "eq(V[%d], %d)" % (list_to_int([i]*indicatorsize, domainsize), i)
  print "**EOF**"


neqtable = filter(lambda (x,y): x < y, product([0,1,2,3,4],[0,1,2,3,4]))

generate_indicator([neqtable], 5, 6)
