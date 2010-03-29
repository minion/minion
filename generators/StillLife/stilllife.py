#!/usr/bin/python

# still life

print "MINION 3"

print "**VARIABLES**"

n=10
usetest=False

print "BOOL l[%d,%d]" %(n+2,n+2)
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

print "bob %d %d" %(len(table), 9)
print " ".join(map(lambda a:str(a), [table[i][j] for i in range(len(table)) for j in range(9) ]))



print "**CONSTRAINTS**"
# kill the edges.

print "sumleq(l[0,_], 0)"
print "sumleq(l[%d,_], 0)"%(n+1)
print "sumleq(l[_,0], 0)"
print "sumleq(l[_,%d], 0)"%(n+1)

for i in range(1, n+1):
    for j in range(1, n+1):
        if usetest:
            print "test(["
        else:
            print "lighttable(["
        for k in range(i-1, i+2):
            for l in range(j-1, j+2):
                if i!= k or j!=l:
                    print "l[%d, %d],"%(k, l)
        print "l[%d, %d]"%(i, j)
        
        if usetest:
            print "])"
        else:
            print "], bob)"
        

print "sumleq(l[_,_], maxvar)"
print "sumgeq(l[_,_], maxvar)"

print "**SEARCH**"
print "MAXIMIZING maxvar"

print "**EOF**"

