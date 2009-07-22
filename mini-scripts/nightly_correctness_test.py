#!/usr/bin/python

# Script to run the nightly correctness testing scripts.

from os import *
from sys import exit
import sendemail

# of course this is bad. The minion directory should be a command line option.
homedir="/cs/home/rsch/pn/"

chdir(homedir+"minion-svn/minion")   #cd /home/pn/minion-svn/minion
system("rm Makefile*")   # get rid of stale makefiles that were causing problems.

retval=0
retval+=system("svn update")
retval+=system("make veryclean")

if retval!=0:
    sendemail.mail("An error occurred when checking out or doing make veryclean.")
    exit(1)

retval+=system("make minion -j2")

if retval!=0:
    sendemail.mail("An error occurred when building minion.")
    exit(1)

retval+=system("export DEBUG=1;make minion -j2")

if retval!=0:
    sendemail.mail("An error occurred when building minion-debug.")
    exit(1)

chdir("test_instances")
retval+=system("./run_tests.sh ../bin/minion")

if retval!=0:
    sendemail.mail("An error occurred when running test_instances/run_tests.sh for minion")
    exit(1)

retval+=system("./run_tests.sh ../bin/minion-debug")

if retval!=0:
    sendemail.mail("An error occurred when running test_instances/run_tests.sh for minion-debug")
    exit(1)

chdir(homedir+"minion-svn/minion")   #cd /home/pn/minion-svn/minion

# This sends its own email when a test fails, so no need to. 
# Do these two test sets in parallel.
system("mini-scripts/testallconstraints.py --numtests=250 --email --minion=bin/minion &") 
# just do 250 random tests for each constraint. Increase when
# we have a faster machine.

system("mini-scripts/testallconstraints.py --numtests=100 --email --minion=bin/minion-debug")

