#!/bin/bash

for i in *.cc;
do
    (../../dominion_build  $i -Wextra -Wall -g -D_GLIBCXX_DEBUG -DDOM_ASSERT &&
./a.out) || { echo Failed' '$i; exit 1; }
done
rm a.out
