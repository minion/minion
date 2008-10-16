#!/usr/bin/python
import sys

print("G := Group( ()")
for line in sys.stdin:
        if(line[0] == "("):
                print(","+line.replace(" ",","))
print(");");
