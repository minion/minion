#!/usr/bin/python

# Script to run the nightly correctness testing scripts.

from os import *
from sys import exit
import sendemail

chdir("/home/pn/minion-svn/minion")   #cd /home/pn/minion-svn/minion

retval=0
retval+=system("svn update")
retval+=system("make veryclean")

if retval!=0:
    sendemail.mail("An error occurred when checking out or doing make veryclean.")
    exit(1)

retval+=system("make minion")

if retval!=0:
    sendemail.mail("An error occurred when building minion.")
    exit(1)

retval+=system("export DEBUG=1;make minion")

if retval!=0:
    sendemail.mail("An error occurred when building minion-debug.")
    exit(1)

retval+=system("cd test_instances; run-tests.sh ../bin/minion")

if retval!=0:
    sendemail.mail("An error occurred when running test_instances/run-tests.sh for minion")
    exit(1)

retval+=system("cd test_instances; run-tests.sh ../bin/minion-debug")

if retval!=0:
    sendemail.mail("An error occurred when running test_instances/run-tests.sh for minion-debug")
    exit(1)

# This sends its own email when a test fails, so no need to. 
system("mini-scripts/testallconstraints.py --numtests=50 --minion=bin/minion") # just do 50 random tests for each constraint.

system("mini-scripts/testallconstraints.py --numtests=50 --minion=bin/minion-debug")

