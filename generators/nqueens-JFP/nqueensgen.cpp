#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
  int nqueens = 8;
  if(argc > 1)
  { nqueens = atoi(argv[1]); }
  printf("MINION 1\n");
  printf("# %d n queen test, model by JFP\n", nqueens);
  printf("0 0 0\n");
  printf("%d\n", 3*nqueens);
  printf("%d %d %d\n", 0, nqueens-1, nqueens);
  
  for(int i = 0; i < nqueens; ++i)
    printf("%d %d 1\n", 0 - i, nqueens - 1 - i);
  
  for(int i = 0; i < nqueens; ++i)
    printf("%d %d 1\n", 0 + i, nqueens -1 + i);
  
  printf("0\n");
  
  // Variable order
  
  printf("[x0");
  for(int i = 2; i < nqueens; i+=2)
  { printf(",x%d", i); }
  for(int i = 1; i < nqueens; i+=2)
  { printf(",x%d", i); }
  
  
  printf("]\n");
  
  printf("[a");
  for(int i = 1; i < nqueens; ++i)
  { printf(",a"); }
  printf("]\n");
  
  printf("0\n3\n");
  
  printf("[[x0");
  for(int i = 1; i < nqueens; ++i)
  { printf(",x%d", i); }
  printf("]]\n");
  
   printf("[[x%d",nqueens);
  for(int i = 1; i < nqueens; ++i)
  { printf(",x%d", i + nqueens); }
  printf("]]\n");

   printf("[[x%d",nqueens*2);
  for(int i = 1; i < nqueens; ++i)
  { printf(",x%d", i + nqueens*2); }
  printf("]]\n");
   

  printf("0\nobjective none\nprint m0\n");
  
  printf("alldiff(m0)\nalldiff(m1)\nalldiff(m2)\n");
  for(int i = 0; i < nqueens; ++i)
  {
    printf("ineq(x%d,x%d,%d)\n",i,i+nqueens,i);
    printf("ineq(x%d,x%d,%d)\n",i+nqueens,i,-i);
    printf("ineq(x%d,x%d,%d)\n",i,i+2*nqueens,-i);
    printf("ineq(x%d,x%d,%d)\n",i+2*nqueens,i,i);
  }

  
}

