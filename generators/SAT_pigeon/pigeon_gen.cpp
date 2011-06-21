#include <iostream>
#include <stdio.h>

using namespace std;

// This will output a simple model of the piegon hole problem, using
// Weighted sum constraints.

int main(int argnum, char* args[])
{
  if(argnum < 2)
  {
    printf("Usage: gen <num of holes>\n");
    exit(1);
  }
  
  int holes = atoi(args[1]);
  int pigeons = holes - 1;
  
  printf("MINION 1\n");
  printf("%d\n", holes * pigeons);
  
  printf("0 0 0 0 [] [] 0 0 0\n");
  printf("objective none\n print all\n");
  
  for(int i = 0; i < holes; ++i)
  {
    printf("watchsumgeq([");
    printf("x%d", i*pigeons);
    for(int j = 1; j < pigeons; ++j)
      printf(",x%d", i*pigeons + j);
    printf("],1)\n");
  }
  
  for(int i = 0; i < holes; ++i)
    for(int j = i + 1; j < holes; ++j)
    {
      for(int x = 0; x < pigeons; ++x)
        printf("weightedsumleq([x%d,x%d],1)\n", i*pigeons+x, j*pigeons+x);
        
    }


}