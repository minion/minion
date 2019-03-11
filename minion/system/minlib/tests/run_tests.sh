#!/ usr / bin / env bash

for
  i in*.cc;
do
(g++ $i - std = gnu++ 11 - Wall - Wextra - DDOM_ASSERT - I../..&&./ a.out) || echo Failed $i done rm
                                                                              a.out
