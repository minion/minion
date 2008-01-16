#!/bin/bash

# A varient of grep. Takes string to find and a list of filenames.
# It prints a list of filenames containing the string.


string=$1

shift

while (( "$#" )); do

grep $string $1 > /dev/null
if [ $? -eq 0 ]; then
  echo $1
fi

shift

done