#!/usr/bin/python
# A script which extracts named columns from a minion tableout file.

import sys
from string import lower
import os.path

# first read in the column header line.

colheads=sys.stdin.readline().strip()

# chuck the # and split on spaces

colheads=colheads[1:].split(" ")

colheads=[c[:-1][1:] for c in colheads] # chop off quotes

colneeded=sys.argv[1:]

# get indices of the needed columns
ind=[]
for c in colneeded:
    for i in range(len(colheads)):
        if(lower(colheads[i])==lower(c)):
            ind.append(i)

assert len(ind)==len(colneeded)

# now process the rest of the lines to stdout.
st=sys.stdin.readline()
while st:
    arr=st.split(" ")
    for i in ind:
        if colheads[i]=="Filename":
            sys.stdout.write(os.path.basename(arr[i])+" ")
        else:
            sys.stdout.write(arr[i]+" ") 
    sys.stdout.write('\n')
    st=sys.stdin.readline()
    

