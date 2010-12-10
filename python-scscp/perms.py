import scscp 

def VariablePermSwap(n, d, var1, var2):
    L = range(1, n*d+1)
    for i in range(0, d):
        L[var1*d + i] = var2*d + i + 1
        L[var2*d + i] = var1*d + i + 1
    return [L]

def ValuePermSwap(n, d, val1, val2):
    L = range(1, n*d+1)
    for i in range(0, n):
        L[i*d + val1] = i*d + val2 + 1
        L[i*d + val2] = i*d + val1 + 1
    return [L]

def VariablePerm(n, d, varlist):
    L = []
    for i in range(len(varlist) - 1):
        L += VariablePermSwap(n, d, varlist[i], varlist[i+1])
    return L

def ValuePerm(n, d, vallist):
    L = []
    for i in range(len(vallist) - 1):
        L += ValuePermSwap(n, d, vallist[i], vallist[i+1])
    return L

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

def GetMinimalImage(G, L):
    global socket
    if socket == False:
        socket = scscp.make_connection()
    command = scscp.build_call("CAJ_MinImage", [ scscp.listint_node(G), scscp.listint_node(L) ] )
    reply = scscp.execute(socket, command)
    return scscp.parse_reply(reply)
