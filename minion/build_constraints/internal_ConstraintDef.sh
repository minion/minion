#!/bin/bash

echo -n \{ $2 , $3, $(($# - 3)), 

case "$#" in
  [4] ) echo -n \{ $4 \};;
  [5] ) echo -n \{ $4 , $5 \};;
  [6] ) echo -n \{ $4 , $5 , $6 \};;
  *   ) echo -n BUG_IN_CONSTRAINTDEF;;
esac

echo , $1 \},