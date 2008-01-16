#include <stdio.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdlib.h>
using namespace std;
int domain = 0;
int k = 0;


vector<int> get_list(int var_num)
{
  vector<int> list(k);
  int pos = k-1;
  while(var_num > 0)
  {
    list[pos] = var_num % domain;
    var_num = var_num / domain;
    pos = pos - 1;
  }
  return list;
}


void print(vector<int> v)
{
  for(int i = 0; i < v.size(); ++i)
    printf("%d, ", v[i]);
  printf("\n");

}

void check_idempotent(vector<int>& v, int varname)
{
  int val = v[0];
  for(int i = 1; i < v.size(); ++i)
    if(v[i] != v[0])
      return;
  printf("eq(x%d,%d)\n",varname,v[0]); 
}

bool not_equal(const vector<int>& x, const vector<int>& y)
{
  for(int i = 0; i < x.size(); ++i)
  {
    if(x[i] == y[i]) 
      return false;
  }
  return true;
}

int main(int argc, char** argv)
{

domain = atoi(argv[1]);
k = atoi(argv[2]);

int vars = (int)pow(domain,k);

printf("MINION 1\n");
printf("0\n0\n0\n %d \n 0 %d %d \n", vars, domain - 1, vars);
printf("0\n [] \n []\n 0\n 1\n");

printf("[ [x0");
for(int i = 1; i < vars; i++) 
  printf(",x%d", i);
printf("] ]\n");

printf("0 objective none\n print m0\n");


int constraints = 0;

for(int i = 0; i < vars; i++)
{
  vector<int> i_vals = get_list(i);
  
  check_idempotent(i_vals, i);
  for(int j = 0; j < vars; j++)
  {
    vector<int> j_vals = get_list(j);
    if(not_equal(i_vals, j_vals))
    {
      printf("diseq(x%d,x%d)\n", i, j);
      constraints ++;
    }
  }
}


//printf("%d\n", constraints);

}