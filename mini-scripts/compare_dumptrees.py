#!/usr/bin/python

import sys
from constraint_test_common import *

# get two filenames
f1=sys.argv[1]
f2=sys.argv[2]

t1=tree()
t1.buildtree(f1)

t2=tree()
t2.buildtree(f2)

# change this to subset if you don't have gac
print t1.equal(t2)

