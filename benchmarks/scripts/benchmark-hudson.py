# First attempt at a benchmarking script
import sys, os, timeit
from threading import Thread
program = sys.argv[1]
benchname = sys.argv[2]
totaltime = 0

class BThread (Thread):
  def __init__ (self,benchmarks):
    Thread.__init__(self)
    self.benchmarks = benchmarks
    self.totaltime = 0

  def run (self):
    for name in self.benchmarks:
      print 'Running ' + name
      if name.endswith('.minion'):
        timer = timeit.Timer('os.system("'+program+' '+name+' &> '+name+'.benchmark.out")','import os')
        time = timer.timeit(1)
        if(time < 5.0):
          # This is a fast experiment
          time = timer.repeat(3,5)
          self.totaltime += min(time)/10.0
        else:
          time = timer.repeat(3,1)
          self.totaltime += min(time)
    

benchmarks = sys.argv[3:]

# split across 2 threads -- TODO: make generic
workers = []
current = BThread([elem for elem in benchmarks if benchmarks.index(elem) % 2 == 0])
workers.append(current)
current.start()
current = BThread([elem for elem in benchmarks if benchmarks.index(elem) % 2 == 1])
workers.append(current)
current.start()

for worker in workers:
  worker.join()
  totaltime += worker.totaltime

FILE = open(benchname + ".benchmark", "w")
FILE.write('YVALUE=%.3f' % totaltime)
FILE.close
