#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

int main(int argc, char** argv)
{
  int size = atoi(argv[1]);
  
  printf("MINION 3\n");
  printf("**VARIABLES**\n");
  printf("DISCRETE PERM[%d] {1..%d}\n", size, size);
  
  int p_vals = 0;
  for(int i = 0; i < size; ++i)
  {
    for(int j = 0; j < i; ++j)
    {
      p_vals++;
      printf("BOOL p%d%d\n", i, j);    
    }
  }
  
  printf("ALIAS parray[%d] = [", p_vals);
  
  bool first_time = true;
  for(int i = 0; i < size; ++i)
  {
    for(int j = 0; j < i; ++j)
    {
      if(first_time)
        first_time = false;
      else
        printf(",");
      printf("p%d%d", i, j);
    }
  }
  printf("]\n");
  
  printf("DISCRETE PERMSUM {0..%d}\n", p_vals);
  
  printf("**SEARCH**\n");
  printf("PRINT [PERM, parray, [PERMSUM]]\n");
  printf("PERMUTATION [PERM]\n");
  printf("VARORDER [PERM, parray, PERMSUM]\n");
  printf("**CONSTRAINTS**\n");
  printf("alldiff(PERM)");
  for(int i = 0; i < size; ++i)
  {
    for(int j = 0; j < i; ++j)
    {
      printf("reify(ineq(PERM[%d], PERM[%d], -1), p%d%d)\n", i, j, i, j);
    }
  }
  
  printf("sumgeq([parray], PERMSUM)\n");
    
  printf("sumleq([parray], PERMSUM)\n");
 
  printf("modulo(PERMSUM, 2, 0)\n"); 
  
  printf("**EOF**\n");
}