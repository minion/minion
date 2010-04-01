#!/usr/bin/python

# still life

print "MINION 3"

print "**VARIABLES**"

n=10
usetest=True
usetable=False
usesum=False


layers=3
life=False

# settings to find pulsar 

if not life:
    # If we're not modelling life, then it's still life, and we have 1 layer.
    layers=1

print "BOOL l[%d,%d,%d]" %(n+4,n+4, layers)

if usesum:
    print "DISCRETE sums[%d,%d,%d] {0..8}"%(n+4, n+4, layers)

print "BOUND maxvar {0..%d}" %(n**2)

print "**TUPLELIST**"

def crossprod(domains, conslist, outlist):
    if domains==[]:
        outlist.append(conslist[:])
        return
    for i in domains[0]:
        ccopy=conslist[:]
        ccopy.append(i)
        crossprod(domains[1:], ccopy, outlist)
    return

cross=[]
crossprod([(0,1) for i in range(9)], [], cross)

table=[]
for l in cross:
    s=sum(l[:8])
    if s>3 or s<2:
        if l[8]==0:
            table.append(l)
    elif s==3:
        if l[8]==1:
            table.append(l)
    else:
        assert s==2
        table.append(l)

print "stilllife %d %d" %(len(table), 9)
print " ".join(map(lambda a:str(a), [table[i][j] for i in range(len(table)) for j in range(9) ]))

cross=[]
crossprod([(0,1) for i in range(10)], [], cross)

table=[]
for l in cross:
    s=sum(l[:8])
    if s>3 or s<2:
        if l[9]==0:
            table.append(l)
    elif s==3:
        if l[9]==1:
            table.append(l)
    else:
        assert s==2
        if l[8]==l[9]:
            table.append(l)

print "life %d %d" %(len(table), 10)
print " ".join(map(lambda a:str(a), [table[i][j] for i in range(len(table)) for j in range(10) ]))

print "sumlink 10 2   0 0 1 0 2 0 2 1 3 1 4 0 5 0 6 0 7 0 8 0"

print "sumlinklife 18 3"
for a in range(9):
    for b in range(2):
        for c in range(2):
            if (a>3 or a<2) and c==0:
                print "%d %d %d"%(a,b,c)
            if a==3 and c==1:
                print "%d %d %d"%(a,b,c)
            if a==2 and b==c:
                print "%d %d %d"%(a,b,c)

print "**CONSTRAINTS**"
# kill the edges.

for i in range(layers):
    print "sumleq(l[0,_,%d], 0)"%i
    print "sumleq(l[1,_,%d], 0)"%i
    print "sumleq(l[%d,_,%d], 0)"%(n+2,i)
    print "sumleq(l[%d,_,%d], 0)"%(n+3,i)
    print "sumleq(l[_,0,%d], 0)"%i
    print "sumleq(l[_,1,%d], 0)"%i
    print "sumleq(l[_,%d,%d], 0)"%(n+2,i)
    print "sumleq(l[_,%d,%d], 0)"%(n+3,i)
    
    if usesum:
        print "sumleq(sums[0,_,%d], 0)"%i
        print "sumleq(sums[%d,_,%d], 0)"%(n+3,i)
        print "sumleq(sums[_,0,%d], 0)"%i
        print "sumleq(sums[_,%d,%d], 0)"%(n+3,i)

for layer in range(layers):
    for i in range(1, n+3):
        for j in range(1, n+3):
            st=""
            for k in range(i-1, i+2):
                for l in range(j-1, j+2):
                    if i!= k or j!=l:
                        st+="l[%d, %d, %d],"%(k, l, layer)
            
            st+="l[%d, %d, %d],"%(i,j, layer)
            if life:
                st+="l[%d, %d, %d],"%(i,j, (layer+1)%layers)
            
            if usetest:
                print "test([%s])"%(st) 
            elif usetable:
                if life:
                    print "lighttable([%s], life)"%(st)
                else:
                    print "lighttable([%s], stilllife)"%(st)
            else:
                assert usesum
                print "sumleq([%s], sums[%d, %d, %d])"%(st, i, j, layer)
                print "sumgeq([%s], sums[%d, %d, %d])"%(st, i, j, layer)
                if not life:
                    print "lighttable([sums[%d, %d, %d], l[%d, %d, %d]], sumlink)"%(i,j,layer, i,j, layer)
                else:
                    print "lighttable([sums[%d, %d, %d], l[%d, %d, %d], l[%d, %d, %d]], sumlinklife)"%(i,j,layer, i,j, layer,i,j,(layer+1)%layers)

if not life:
    print "sumleq(l[_,_,0], maxvar)"
    print "sumgeq(l[_,_,0], maxvar)"
    
    print "**SEARCH**"
    print "MAXIMIZING maxvar"
else:
    # to be interesting the first layer should have something on it. 
    # two dots will just die, so at least 3 are required.
    #print "sumgeq(l[_,_,0], 3)"
    print "sumleq(l[_,_,_], maxvar)"
    print "sumgeq(l[_,_,_], maxvar)"
    
    print "**SEARCH**"
    print "MAXIMIZING maxvar"
    

print "PRINT [["
for layer in range(layers):
    for i in range(0, n+4):
        for j in range(0, n+4):
            print "l[%d, %d, %d],"%(i,j,layer)
        print "],["
    print "],["

print "]]"

# val order
print "VARORDER [l[_,_,0]]"   # should only need to branch on layer 0.
print "VALORDER ["+(" ".join(["d" for i in range((n+4)**2)]))+" a]"

print "**EOF**"

