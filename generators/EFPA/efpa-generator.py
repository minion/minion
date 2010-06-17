#!/usr/bin/python
# Generate EFPA problems for minion

# equidistant frequency permutation arrays.

# By default this version expects minion-static to be in the working dir,
# and will run it and parse the output.

import math

import sys, os, getopt

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["gcc", "occurrence", "q=", "lambda=", "d=", "numcodes=", "implied", "table", "fillin", "fillingcc", "scalarprod", "snakelex", "snakeorder"])

if len(other)!=1:
    print "Usage: efpa-generator.py --q=<alphabet size>"
    print "--lambda=<number of each symbol in each codeword>"
    print "--d=<Hamming distance between pairs of codewords>"
    print "--numcodes=<number of codewords>"
    print "--occurrence (use occurrence constraints, recommended)"
    print "--gcc (use GCC constraints, not recommended)"
    print "--implied (add "
    
    sys.exit(1)

q=3     # size of alphabet
lam=3   # number of each symbol
d=4     # hamming distance between two adjacent codes.
numcodes=7     # number of codes.
gcc=False
occurrence=False
implied=False
tables=False
fillins=False
fillingcc=False
scalarprod=False
snakelex=False
snakeorder=False
runminion=True
minion="./minion-static"

for i in optargs:
    (a1, a2)=i
    if a1=="--gcc":
        gcc=True
    elif a1=="--occurrence":
        occurrence=True
    elif a1=="--q":
        q=int(a2)
    elif a1=="--lambda":
        lam=int(a2)
    elif a1=="--d":
        d=int(a2)
    elif a1=="--numcodes":
        numcodes=int(a2)
    elif a1=="--implied":
        implied=True
    elif a1=="--table":
        tables=True
    elif a1=="--fillin":
        fillins=True
    elif a1=="--fillingcc":
        fillingcc=True
    elif a1=="--scalarprod":
        scalarprod=True
    elif a1=="--snakelex":
        snakelex=True
    elif a1=="--snakeorder":
        snakeorder=True

print "MINION 3"
print "**VARIABLES**"

print "DISCRETE codes[%d, %d] {0..%d}"%(numcodes, q*lam, q-1)

if tables:
    print "**TUPLELIST**"
    print "reifydiseq %d 3"% (q*q)
    for i in range(q):
        for j in range(q):
            if i==j:
                t=0
            else:
                t=1
            print "%d %d %d" % (i, j, t)

print "**CONSTRAINTS**"

#for i in range(numcodes-1):
#    print "hamming([codes[%d,_]], [codes[%d,_]], %d)"%(i, i+1, d)

boolmap=dict()
count =0
for i in range(numcodes):
    for k in range(i+1, numcodes):
        print "**VARIABLES**"
        print "BOOL diff%d[%d]"%(count, q*lam)
        print "**CONSTRAINTS**"
        print "sumleq([diff%d], %d)"%(count, d)
        print "sumgeq([diff%d], %d)"%(count, d)
        for j in range(q*lam):
            if not tables:
                print "reify(diseq(codes[%d, %d], codes[%d, %d]), diff%d[%d])"%(i, j, k, j, count, j)
            else:
                print "table([codes[%d, %d], codes[%d, %d], diff%d[%d]], reifydiseq)"%(i, j, k, j, count, j)
        boolmap[(i,k)]=count
        boolmap[(k,i)]=count
        count+=1

if implied:
    # transitive closure on = constraints that emerge when reify bools are set to 0.
    for i in range(numcodes):
        for j in range(numcodes):
            for k in range(numcodes):
                if k!=i and k!=j and i!=j:
                    # for each symbol in the codewords
                    for s in range(q*lam):
                        # now we have three bools. If i and j are set to 0, the other
                        # should be set to 0.
                        print "reifyimply(sumgeq([diff%d[%d], diff%d[%d]], 1), diff%d[%d])"%(boolmap[(i,j)], s, boolmap[(j,k)], s, boolmap[(i,k)], s)

implied2=False  
if implied2:
    # appears to make no difference.
    print "gcc([codes], %s, %s)"%(str([i for i in range(q)]), str([lam*numcodes for i in range(q)]))

for i in range(numcodes):
    if gcc:
        print "gcc([codes[%d,_]], %s, %s)"%(i, str(range(q)), str([lam for j in range(q)]))
    if occurrence:
        for val in range(q):
            print "occurrence([codes[%d,_]], %d, %d)"%(i, val, lam)

if snakelex:
    m0="codes"  # name of the matrix to act on.
    #/**
    # * This assumes a matrix called m0 with "row" rows and "column" columns.
    # * There a matrix "mRev" which is an alias to "m0" with eahc variable in a row in the reverse order.
    # * 
    # * The constraints produced in this file are suitable for a variable search ordering which starts at the top left of a matrix
    # * then traverses the first row left to right.  After this it moves backwards, right to left, along the second row.  This pattern
    # * repeats until all the rows, and variables, have been used.
    # * 
    # * If it is more suitable to have the variables ordered down the first column first then a different set of constraints should be used.
    # */
    
    #//String to store minion input file.
    output = ""
    
    #//number of rows
    row = numcodes
    #//number of columns
    column = q*lam
    
    #/**
    # *  **variables**
    # *  
    # *  We first define a reversed matrix to "m0" called "mRev".
    # */
    output += "**VARIABLES**\n"
    output += "# aliases\n"
    output += "ALIAS mRev["+str(row)+","+str(column)+"] = ["
    
    for i in range(row):
        output+="["
        for j in range(column-1, 0, -1):
            output+= "%s[%d, %d], "%(m0,i,j)
        if i == (row-1):
            output += m0+"["+str(i)+", 0]]]\n"
        else:
            output += m0+"["+str(i)+", 0]],"
    
    #/**
    # * This section creates the row based lex constraints followed by the column based ones
    # */
    #//rows
    output+="**CONSTRAINTS**\n"
    for i in range(row-1):
        if ((i + 1) % 2 != 0):
            output += "lexleq("+m0+"[" + str(i) + ",_], "+m0+"[" + str(i + 1) + ",_])\n"
            if ((i + 2) <= row - 1):
                output += "lexleq("+m0+"[" + str(i) + ",_], "+m0+"["+ str(i + 2) + ",_])\n";
        else:
            output += "lexleq(mRev[" + str(i) + ",_], mRev["+str(i+1)+ ",_])\n";
            if ((i + 2) <= row - 1):
                output += "lexleq(mRev[" + str(i) + ",_], mRev["+str(i+2)+ ",_])\n"
    #//columns
    for i in range(column-1):
        output += "lexleq(["
        for j in range(row):
            if ((j + 1) % 2 != 0):
                output += m0+"["+str(j) +","+str(i)+"],"
            else:
                output += m0+"["+str(j) +","+str(i+1)+"],"
        output = output[:-1]+ "],["
        for j in range(row):
            if ((j + 1) % 2 != 0):
                output += m0+"["+str(j) +","+str(i+1)+"],"
            else:
                output += m0+"["+str(j) +","+str(i)+"],"
        output = output[:-1]+"])\n"
    print output
else:
    # add double-lex symmetry breaking.
    for i in range(numcodes-1):
        print "lexless([codes[%d,_]], [codes[%d,_]])"%(i, i+1)
    
    for i in range(q*lam -1):
        print "lexleq([codes[_,%d]], [codes[_,%d]])"%(i, i+1)

#Fill in the first row.
for i in range(q*lam):
    print "eq(codes[0, %d], %d)"%(i, i//lam)

if fillins:
    # Fill in parts of the second row based on lambda and d
    diff=d//2
    fillin=lam-diff
    if fillin>0:
        for i in range(q):
            if i==0:
                for j in range(fillin):
                    print "eq(codes[1, %d], %d)"%(i*lam+j, i)
            elif i==q-1:
                for j in range(fillin):
                    print "eq(codes[1, %d], %d)"%((i+1)*lam-j-1, i)
            else:
                v=""
                for j in range(lam): v+="codes[1, %d],"%(i*lam+j)
                print "occurrencegeq([%s], %d, %d)"%(v[:-1], i, fillin)
        for i in range(q):
                v=""
                for j in range(lam): v+="codes[1, %d],"%(i*lam+j)
                print "occurrencegeq([%s], %d, %d)"%(v[:-1], i, fillin)
        # all other rows
        for row in range(1, numcodes):
            if fillingcc:
                print "**VARIABLES**"
                print "DISCRETE cap%d[%d, %d] {%d..%d}"%(row, q, q, 0, lam)
                # cap<row num>[<block>,<value>] is the number of value in the block
                print "**CONSTRAINTS**"
            ## join the q gccs together through the cap variables.
            for i in range(q):
                v=""
                for j in range(lam): v+="codes[%d, %d],"%(row, i*lam+j)
                if fillingcc:
                    print "gcc([%s], %s, cap%d[%d,_])"%(v[:-1], str([dx for dx in range(q)]), row, i)
                    print "ineq(%d, cap%d[%d, %d], 0)"%(fillin, row, i, i)
                    print "sumleq([cap%d[_,%d]], %d)"%(row, i, lam)
                    print "sumgeq([cap%d[_,%d]], %d)"%(row, i, lam)
                else:
                    print "occurrencegeq([%s], %d, %d)"%(v[:-1], i, fillin)

if scalarprod:
    # for each row, Make a 0/1 vector for the occurrence of each value in the row
    print "**VARIABLES**"
    print "BOOL occs[%d, %d, %d]"%(numcodes, q, q*lam)
    print "**CONSTRAINTS**"
    # occs[<row>, <value>, <row index>]
    
    for row in range(numcodes):
        for val in range(q):
            for pos in range(q*lam):
                # channel
                print "reify(eq(codes[%d, %d], %d),occs[%d, %d, %d])"%(row, pos, val, row, val, pos)
    
    # for each pair of rows, and each value.
    # do the funky thang.
    for row1 in range(numcodes):
        for row2 in range(row1+1, numcodes):
            print "**VARIABLES**"
            print "BOOL product_%d_%d[%d, %d]"%(row1, row2, q, q*lam)
            print "**CONSTRAINTS**"
            for val in range(q):
                for pos in range(q*lam):
                    print "product(occs[%d, %d, %d], occs[%d, %d, %d], product_%d_%d[%d,%d])"%(row1, val, pos, row2, val, pos, row1, row2, val, pos)
                print "sumgeq([product_%d_%d[%d,_]], %d)"%(row1, row2, val, lam-(d//2))
    
print "**SEARCH**"

#rows
if snakeorder:
    row=numcodes
    column=q*lam
    m0="codes"
    output = "VARORDER ["
    for i in range(row):
        if (i + 1) % 2 != 0:
            for j in range(column):
                output += m0+"[" + str(i) + "," + str(j) + "],"
        else:
            for j in range(column-1, -1, -1):
                output += m0+"[" + str(i) + "," + str(j) + "],"
    output = output[:-1]
    for i in range(count):
        output+= ",diff%d"%i
    output+="]\n"
    print output

if not snakeorder:
    print "VARORDER [codes"
    for i in range(count):
        print ",diff%d"%i
    if scalarprod:
        print ",occs"
        for row1 in range(numcodes):
            for row2 in range(row1+1, numcodes):
                print ",product_%d_%d"%(row1, row2)
    print "]"

#columns
#print "VARORDER ["
#for i in range(q*lam-1):
#    print "codes[_,%d],"%i 
#print "codes[_,%d]]"%(q*lam-1)

#aux vars columnwise
#print "VARORDER ["
#st=""
#for i in range(q*lam):
#    for j in range(numcodes):
#        for k in range(j+1, numcodes):
#            st+= "diff%d[%d],"%(boolmap[(j,k)], i)
#print st+"codes]"

# zigzag rows starting at row 2
#st=""
#for i in range(numcodes):
#    # odd rows go left to right
#    if i%2==0:
#        for j in range(q*lam):
#            st+="codes[%d, %d],"%(i, j)
#    else:
#        for j in range(q*lam-1, -1, -1):
#            st+="codes[%d, %d],"%(i, j)
#print "VARORDER [%s"%(st)
#for i in range(count-1):
#    print "diff%d,"%i
#print "diff%d]"%(count-1)

print "PRINT [codes]"

print "**EOF**"

