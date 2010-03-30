#!/usr/bin/python

# still life

print "MINION 3"

print "**VARIABLES**"

n=17
usetest=True
usetable=False
usesum=False


layers=3
life=True

# settings to find pulsar 

if not life:
    # If we're not modelling life, then it's still life, and we have 1 layer.
    layers=1

print "BOOL l[%d,%d,%d]" %(n+2,n+2, layers)

if usesum:
    print "DISCRETE sums[%d,%d,%d] {0..8}"%(n+2, n+2, layers)

if not life:
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

print "life %d %d" %(len(table), 9)
print " ".join(map(lambda a:str(a), [table[i][j] for i in range(len(table)) for j in range(10) ]))

print "sumlink 10 2   0 0 1 0 2 0 2 1 3 1 4 0 5 0 6 0 7 0 8 0"


print "**CONSTRAINTS**"
# kill the edges.

for i in range(layers):
    print "sumleq(l[0,_,%d], 0)"%i
    print "sumleq(l[%d,_,%d], 0)"%(n+1,i)
    print "sumleq(l[_,0,%d], 0)"%i
    print "sumleq(l[_,%d,%d], 0)"%(n+1,i)
    
    if usesum:
        print "sumleq(sums[0,_,%d], 0)"%i
        print "sumleq(sums[%d,_,%d], 0)"%(n+1,i)
        print "sumleq(sums[_,0,%d], 0)"%i
        print "sumleq(sums[_,%d,%d], 0)"%(n+1,i)

for layer in range(layers):
    for i in range(1, n+1):
        for j in range(1, n+1):
            if usetest:
                print "test(["
            elif usetable:
                print "lighttable(["
            
            st=""
            for k in range(i-1, i+2):
                for l in range(j-1, j+2):
                    if i!= k or j!=l:
                        st+="l[%d, %d, %d],"%(k, l, layer)
            
            st+="l[%d, %d, %d],"%(i,j, layer)
            if life:
                if layer== (layers-1):
                    st+="l[%d, %d, %d],"%(i,j, 0)
                else:
                    st+="l[%d, %d, %d],"%(i,j, layer+1)
            
            if usetest:
                print "test([%s])"%(st) 
            elif usetable:
                if life:
                    print "lighttable([%s], life)"%(st)
                else:
                    print "lighttable([%s], stilllife)"%(st)
            else:
                assert usesum
                assert not life
                print "sumleq([%s], sums[%d, %d])"%(st, i, j)
                print "sumgeq([%s], sums[%d, %d])"%(st, i, j)
                print "lighttable([sums[%d, %d], l[%d, %d]], sumlink)"%(i,j,i,j)
            
if not life:
    print "sumleq(l[_,_,], maxvar)"
    print "sumgeq(l[_,_,], maxvar)"

print "**SEARCH**"
print "MAXIMIZING maxvar"

print "**EOF**"

