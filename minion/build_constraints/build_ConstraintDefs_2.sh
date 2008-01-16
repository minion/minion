#!/bin/bash


echo -n \{ $2 , $3, $4 ,

case "$4" in
  [1] ) echo -n \{ $6 \};;
  [2] ) echo -n \{ $6 , $7 \};;
  [3] ) echo -n \{ $6 , $7 , $8 \};;
  *   ) echo -n BUG_IN_CONSTRAINTDEF;;
esac

echo , $1 \},