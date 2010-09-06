#!/usr/bin/python
# This script is just a thin wrapper on top of the E' model in implied.eprime
import sys,os,getopt

minionbin="./minion"

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["q=", "lambda=", "d=", "numcodes=", "numsols=", "timelimit="])

if len(other)!=1:
    print "Usage: efpa.py --q=<alphabet size> --lambda=<occurrences of each symbol>"
    print "   --d=<Hamming distance> --numcodes=<number of codewords>"
    sys.exit(1)

q=-1     # size of alphabet
lam=-1   # number of each symbol
d=-1     # hamming distance between two adjacent codes.
numcodes=-1     # number of codes.
numsols=1
timelimit=86400    # one day

for (a1,a2) in optargs:
    if a1=="--q":
        q=int(a2)
    elif a1=="--lambda":
        lam=int(a2)
    elif a1=="--d":
        d=int(a2)
    elif a1=="--numcodes":
        numcodes=int(a2)
    elif a1=="--numsols":
        numsols=int(a2)
    elif a1=="--timelimit":
        timelimit=int(a2)

# Make a parameter file

if (q==-1) or (lam==-1) or (d==-1) or (numcodes==-1):
    print "Not all of the four parameters were specified."
    print "Usage: efpa.py --q=<alphabet size> --lambda=<occurrences of each symbol>"
    print "   --d=<Hamming distance> --numcodes=<number of codewords>"
    sys.exit(1)

f=open("efpa-temp.param","w") 

f.write("language ESSENCE' 1.b.a\n")

f.write("letting q be %d\n"%(q))
f.write("letting d be %d\n"%(d))
f.write("letting lambda be %d\n"%(lam))
f.write("letting noCodes be %d\n"%(numcodes))
f.write("letting valuesArray be %s\n"%(str(range(1,q+1))))
f.write("letting occsArray be %s\n"%(str([lam for i in range(q)])))
f.close()

os.system("java -jar tailor.jar -silent implied.eprime efpa-temp.param")

minout1=os.popen("%s efpa-temp.param.minion -noresume -timelimit %d -sollimit %d"%(minionbin, timelimit, numsols)).readlines()

nodeout=False

# Check if 100000 nodes was enough
nodes1=filter(lambda a: a[0:12]=="Total Nodes:", minout1)
n1=int(nodes1[0].partition(":")[2])
#if n1==nodelimit:
#    nodeout=True

# Extract blocks of adjacent sol lines
sols=[]
while minout1:
    if minout1[0][0:4]=="Sol:":
        # parse a solution
        sol=[]
        while minout1 and minout1[0][0:4]=="Sol:":
            sol.append(minout1[0])
            minout1=minout1[1:]
        sol=sol[:numcodes]
        sols.append(sol)
    else:
        # throw a line away.
        minout1=minout1[1:]

#sol1=filter(lambda a: a[0:4]=="Sol:", minout1)

if len(sols)==0:
    print "No solution found."
    if nodeout or timeout:
        print "Solver reached node or time limit. There may be a solution, but it was not found"
    sys.exit(0)

for sol1 in sols:
    # parse it
    sol2=[]
    for line in sol1:
        sol2.append([int(a)-1 for a in line.split()[1:]])
    
    print "Solution found. Codewords:"
    for l in sol2:
        print ", ".join([str(a) for a in l])
    
    print ""
    
    pad=numcodes*2+1
    
    print "".join([st.ljust(pad) for st in ["Position:"]+[str(a) for a in range(1,q*lam+1)]])
    print ""
    for symbol in range(q):
        st="%d"%(symbol)+" "*(pad-1)
        for position in range(q*lam):
            # print numbers of codewords that have symbol at position
            cwdset=[]
            for codeword in range(numcodes):
                if sol2[codeword][position]==symbol:
                    cwdset.append(codeword+1)
            
            st+=(",".join([str(a) for a in cwdset])).ljust(pad)
            
        
        print st
    print ""
    print ""

print "%d solutions found." %(len(sols))
