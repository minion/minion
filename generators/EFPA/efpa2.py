#!/usr/bin/python
# This script is just a thin wrapper on top of the E' model in implied.eprime
import sys,os,getopt

minionbin="./minion"

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["q=", "lambda=", "d=", "numcodes=", "numsols=", "timelimit=", "fillin="])

if len(other)!=1:
    print("Usage: efpa.py --q=<alphabet size> --lambda=<occurrences of each symbol>")
    print("   --d=<Hamming distance> --numcodes=<number of codewords>")
    sys.exit(1)

q=-1     # size of alphabet
lam=-1   # number of each symbol
d=-1     # hamming distance between two adjacent codes.
numcodes=-1     # number of codes.
numsols=1
timelimit=86400    # one day
fillinfile=""

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
    elif a1=="--fillin":
        fillinfile=a2

f=open("temp.minion", "w")

f.write("MINION 3\n")
f.write("**VARIABLES**\n")

f.write("DISCRETE codes[%d, %d] {0..%d}\n"%(numcodes, q*lam, q-1))


f.write("**CONSTRAINTS**\n")


boolmap=dict()
count =0
for i in range(numcodes):
    for k in range(i+1, numcodes):
        f.write("**VARIABLES**\n")
        f.write("BOOL diff%d[%d]\n"%(count, q*lam))
        f.write("**CONSTRAINTS**\n")
        f.write("sumleq([diff%d], %d)\n"%(count, d))
        f.write("sumgeq([diff%d], %d)\n"%(count, d))
        for j in range(q*lam):
            f.write("reify(diseq(codes[%d, %d], codes[%d, %d]), diff%d[%d])\n"%(i, j, k, j, count, j))
        boolmap[(i,k)]=count
        boolmap[(k,i)]=count
        count+=1

for i in range(numcodes):
    f.write("gcc([codes[%d,_]], %s, %s)\n"%(i, str(range(q)), str([lam for j in range(q)])))

# parse the fill in arrays....

if fillinfile!="":
    import csv
    input = open(fillinfile, 'rb')
    reader = csv.reader(input)
    fillins=[]
    for line in reader:
        fillins.append([int(i) for i in line])
    
    for x in range(len(fillins)):
        line=fillins[x]
        for y in range(len(line)):
            if(line[y]!=-1):
                f.write("eq(codes[%d, %d], %d)\n"%(x,y,line[y]))

f.write("**EOF**\n")
f.close()


minout1=os.popen("%s temp.minion -noresume -timelimit %d -sollimit %d"%(minionbin, timelimit, numsols)).readlines()

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
    print("No solution found.")
    #if nodeout or timeout:
    #    print("Solver reached node or time limit. There may be a solution, but it was not found")
    sys.exit(0)

for sol1 in sols:
    # parse it
    sol2=[]
    for line in sol1:
        sol2.append([int(a) for a in line.split()[1:]])
    
    print("Solution found. Codewords:")
    for l in sol2:
        print(", ".join([str(a) for a in l]))
    
    print("")
    
    pad=numcodes*2+1
    
    print("".join([st.ljust(pad) for st in ["Position:"]+[str(a) for a in range(1,q*lam+1)]]))
    print("")
    for symbol in range(q):
        st="%d"%(symbol)+" "*(pad-1)
        for position in range(q*lam):
            # print numbers of codewords that have symbol at position
            cwdset=[]
            for codeword in range(numcodes):
                if sol2[codeword][position]==symbol:
                    cwdset.append(codeword+1)
            
            st+=(",".join([str(a) for a in cwdset])).ljust(pad)
            
        
        print(st)
    print("")
    print("")

print("%d solutions found." %(len(sols)))
    
        


