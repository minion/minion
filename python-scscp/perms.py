import scscp 
from VarValToDomain import *

def groupFromTuples(tuples):
    edges = []
    for t in tuples:
      edge = []
      for i in range(len(t)):
        edge.append(get_lit(i, t[i]))
      edges.append(edge)

    CAJ_GraphGenerators

def VariablePermSwap(var1, var2):
    L = range(1, get_total_litcount() + 1)
    assert(get_domain(var1) == get_domain(var2))
    for i in get_domain(var1):
        L[get_lit(var1, i) - 1] = get_lit(var2, i)
        L[get_lit(var2, i) - 1] = get_lit(var1, i)
    return [L]

def VariablePermSwapList(p):
    L = range(1, get_total_litcount() + 1)
    for i in p[1:]:
      assert(get_domain(p[0]) == get_domain(i))
    
    for i in range(len(p)):
      for j in get_domain(p[0]):
          L[get_lit(i, j) - 1] = get_lit(p[i], j)
    return [L]

def ValuePermSwap(val1, val2):
    L = range(1, get_total_litcount() + 1)
    for i in range(get_total_varcount()):
        L[get_lit(i, val1) - 1] = get_lit(i, val2)
        L[get_lit(i, val2) - 1] = get_lit(i, val1)
    return [L]

def ValuePermSwapList(p):
    L = range(1, get_total_litcount() + 1)
    for i in range(get_total_varcount()):
      for j in get_domain(i):
          L[get_lit(i, j) - 1] = get_lit(i, p[j])
    return [L]

def VariableTotalPerm(varlist):
    L = []
    for i in range(len(varlist) - 1):
        L += VariablePermSwap(varlist[i], varlist[i+1])
    return L

def ValueTotalPerm(vallist):
    L = []
    for i in range(len(vallist) - 1):
        L += ValuePermSwap(vallist[i], vallist[i+1])
    return L

def LiteralPermSwapList(p):
    L = range(1, get_total_litcount() + 1)
    for ( lit1,lit2 ) in p.iteritems():
        L[get_lit(*lit1) - 1] = get_lit(*lit2)
    
    return [L]

def InvPerm(l):
   R = list(l)
   for i in range(len(l)):
       R[l[i] - 1] = i + 1
   return R

def PadPerm(m, length):
    return m + [ i + 1 + len(m) for i in range(length - len(m)) ]

def MultPerm(m1, m2):
   length = max(len(m1), len(m2))
   newm1 = PadPerm(m1, length)
   newm2 = PadPerm(m2, length)

   return [ newm2[newm1[i] - 1] for i in range(length) ]

socket = False

def GetGraphGens(T,n):
    global socket
    if socket == False:
        socket = scscp.make_connection()
    command = scscp.build_call("CAJ_GetGraphGens", [ scscp.listint_node(T), scscp.int_node(n) ] )
    reply = scscp.execute(socket, command)
    return scscp.parse_reply(reply)

def GetMinimalImage(G, L):
    global socket
    if socket == False:
        socket = scscp.make_connection()
    command = scscp.build_call("CAJ_MinImage", [ scscp.listint_node(G), scscp.listint_node(L) ] )
    reply = scscp.execute(socket, command)
    return scscp.parse_reply(reply)

def GetGroupSize(G):
    global socket
    if socket == False:
        socket = scscp.make_connection()
    command = scscp.build_call("CAJ_GroupSize", [ scscp.listint_node(G) ] )
    reply = scscp.execute(socket, command)
    return scscp.parse_reply(reply)
