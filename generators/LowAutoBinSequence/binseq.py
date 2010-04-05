#!/usr/bin/python

n=25

grouptwo=True
groupthree=False

# true, use lighttable, false, use test constraint.
usetable=True
tablecon="lighttable"

symbreak=True


print "MINION 3"

print "**VARIABLES**"

print "DISCRETE seq[%d] {-1..1}"%n
print "BOUND optvar {0..%d}" % (n*n*n)


# non-periodic (open) boundary conditions.

print "DISCRETE ck[%d] {%d..%d}"%(n-1, -n, n)
print "DISCRETE cksquared[%d] {0..%d}"%(n-1, n*n)

print "**SEARCH**"

print "VARORDER ["
for i in range(n//2):
    print "seq[%d], seq[%d],"%(i, n-i-1)
    
if (n//2)*2 < n:
    print "seq[%d],"%(n//2)
print "]"


print "**TUPLELIST**"

sq=[]
for i in range(-n, n+1):
    sq.append(i)
    sq.append(i*i)

print "square %d 2"%(len(sq)//2)
print reduce(lambda a,b: str(a)+" "+str(b), sq)

twoprod=[]
for a in [-1, 1]:
    for b in [-1,1]:
        for c in [-1, 1]:
            for d in [-1, 1]:
                twoprod.append((a,b,c,d, (a*b)+(c*d)))

print "twoprod %d 5"%len(twoprod)
print " ".join(map(lambda a:" ".join(map(lambda b:str(b),a)), twoprod))

threeprod=[]
for a in [-1, 1]:
    for b in [-1,1]:
        for c in [-1, 1]:
            for d in [-1, 1]:
                for e in [-1, 1]:
                    for f in [-1, 1]:
                        threeprod.append((a,b,c,d,e,f, (a*b)+(c*d)+(e*f)))

print "threeprod %d 7"%len(threeprod)
print " ".join(map(lambda a:" ".join(map(lambda b:str(b),a)), threeprod))



print "**SEARCH**"
print "MINIMISING optvar"

print "**CONSTRAINTS**"
for i in range(n):
    print "diseq(seq[%d], 0)" % i



for k in range(1, n):
    if grouptwo:
        # group the sum of two products together.
        numprodvars=(n-k)//2
        if numprodvars*2 < n-k:
            numprodvars+=1
        
        print "**VARIABLES**"
        print "DISCRETE prod%d[%d] {-2..2}"%(k, numprodvars)
        print "**CONSTRAINTS**"
        
        print "sumgeq([prod%d[_]], ck[%d])" %(k, k-1)
        print "sumleq([prod%d[_]], ck[%d])" %(k, k-1)
        
        # knock out 
        
        for i in range(0, (n-k)//2):
            startidx=i*2
            print "diseq(prod%d[%d], -1)" %(k, i)
            print "diseq(prod%d[%d], 1)" %(k, i)
            if usetable:
                print "lighttable([seq[%d], seq[%d], seq[%d], seq[%d], prod%d[%d]], twoprod)"%(startidx, startidx+k, startidx+1, startidx+k+1, k, i)
            else:
                # use the test constraint.
                print "test([seq[%d], seq[%d], seq[%d], seq[%d], prod%d[%d]])" %(startidx, startidx+k, startidx+1, startidx+k+1, k, i)
            
        if ((n-k)//2)*2 < n-k:
            startidx=n-k-1
            print "product(seq[%d], seq[%d], prod%d[%d])"%(startidx, startidx+k, k, numprodvars-1)
            print "diseq(prod%d[%d], 0)"%(k, numprodvars-1)
    elif groupthree:
        # group the sum of three products together.
        # may need one or two extra 'product' constraints to fill in.
        numprodvars=(n-k)//3
        numprodvars+=(n-k-(numprodvars*3))  # extra ones to fill in the end.
        
        print "**VARIABLES**"
        print "DISCRETE prod%d[%d] {-3..3}"%(k, numprodvars)
        print "**CONSTRAINTS**"
        for i in range((n-k)//3):
            print "diseq(prod%d[%d], -2)" %(k, i)
            print "diseq(prod%d[%d], 0)" %(k, i)
            print "diseq(prod%d[%d], 2)" %(k, i)
        
        for i in range((n-k)//3, numprodvars):
            # rest are -1, 1
            print "diseq(prod%d[%d], -3)" %(k, i)
            print "diseq(prod%d[%d], -2)" %(k, i)
            print "diseq(prod%d[%d], 0)" %(k, i)
            print "diseq(prod%d[%d], 2)" %(k, i)
            print "diseq(prod%d[%d], 3)" %(k, i)
        
        print "sumgeq([prod%d[_]], ck[%d])" %(k, k-1)
        print "sumleq([prod%d[_]], ck[%d])" %(k, k-1)
        
        for i in range(0, (n-k)//3):
            startidx=i*3
            if usetable:
                print "lighttable([seq[%d], seq[%d], seq[%d], seq[%d], seq[%d], seq[%d], prod%d[%d]], threeprod)"\
                %(startidx, startidx+k, startidx+1, startidx+k+1, startidx+2, startidx+k+2, k, i)
            else:
                # use the test constraint.
                print "test([seq[%d], seq[%d], seq[%d], seq[%d], seq[%d], seq[%d], prod%d[%d]])"\
                %(startidx, startidx+k, startidx+1, startidx+k+1, startidx+2, startidx+k+2, k, i)
        
        if ((n-k)//3)*3 < n-k:
            diff = n-k - ((n-k)//3)*3
            for i in range(1, diff+1):
                startidx=n-k-i
                print "product(seq[%d], seq[%d], prod%d[%d])"%(startidx, startidx+k, k, numprodvars-i)
    else:
        print "**VARIABLES**"
        print "DISCRETE prod%d[%d] {-1..1}"%(k, n-k) 
        
        print "**CONSTRAINTS**"
        for startidx in range(0, n-k):
            #seq[startidx]*seq[startidx+k]
            print "product(seq[%d], seq[%d], prod%d[%d])"%(startidx, startidx+k, k, startidx)
        
        print "sumgeq([prod%d[_]], ck[%d])" %(k, k-1)
        print "sumleq([prod%d[_]], ck[%d])" %(k, k-1)
        

# symmetry breaking
if symbreak:
    print "**VARIABLES**"
    print "BOOL seq2[%d]" %n
    print "**CONSTRAINTS**"
    
    for i in range(n):
        print "reify(eq(seq[%d],1), seq2[%d])"%(i,i)
    
    # Reverse order
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join(["seq2[%d]"%(i) for i in range(n-1, -1, -1) ]))+"])"
    
    # Negation of each number
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join(["!seq2[%d]"%(i) for i in range(n) ]))+"])"
    
    # Negation of even positions 
    ls=[]
    for i in range(n):
        if i%2 == 1:
            ls.append("seq2[%d]"%i)
        else:
            ls.append("!seq2[%d]"%i)
    
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join(ls))+"])"
    
    # Negation of odd positions 
    ls_odd=[]
    for i in range(n):
        if i%2 == 1:
            ls_odd.append("!seq2[%d]"%i)
        else:
            ls_odd.append("seq2[%d]"%i)
    
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join(ls_odd))+"])"
    # reverse negation
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join(["!seq2[%d]"%(i) for i in range(n-1, -1, -1) ]))+"])"
    
    # reverse negation of odds
    
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join([ls_odd[i] for i in range(n-1, -1, -1)]))+"])"
    
    # reverse negation of evens
    print "lexleq(["+(",".join(["seq2[%d]"%(i) for i in range(n) ]))+"],["+(",".join([ls[i] for i in range(n-1, -1, -1)]))+"])"


for i in range(n-1):
    #print "product(ck[%d], ck[%d], cksquared[%d])"%(i,i,i)
    print "lighttable([ck[%d], cksquared[%d]], square)"%(i,i)

print "sumgeq([cksquared[_]], optvar)"
print "sumleq([cksquared[_]], optvar)"

print "**EOF**"

