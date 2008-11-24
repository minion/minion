# First attempt at a benchmarking script
import sys, os, timeit
program = sys.argv[1]
benchmarks = sys.argv[2]
totaltime = 0
for root, dirs, files in os.walk(benchmarks):
  for name in files:
    if name.endswith('.minion') or name.endswith('.minion.bz2'):
      timer = timeit.Timer('os.system("'+program+' '+root+'/'+name+' &> /dev/null")','import os')
      time = timer.timeit(1)
      if(time < 5.0):
        # This is a fast experiment
        time = timer.repeat(3,5)
        mintime = min(time)/10.0
      else:
        time = timer.repeat(3,1)
        mintime = min(time)
      print name.ljust(30) + '%7.3f' % mintime
      totaltime = totaltime + mintime
print "total".ljust(30) + '%7.3f' % totaltime
