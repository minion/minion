#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

using namespace std;


vector<pair<int, int> > reverseList(vector<pair<int, int> >& v, int n)
{
  sort(v.begin(), v.end());
  
  vector<pair<int, int> > ret_val;
  
  for(int i = 0; i < n; ++i)
  {
    for(int j = 0; j < n; ++j)
    {
      if(!binary_search(v.begin(), v.end(), make_pair(i,j)))
        ret_val.push_back(make_pair(i,j));
    }
  }
  
  return ret_val;
}

vector<pair<int, int> > makeGrid(int x, int y)
{
  vector<pair<int, int> > grid;
  
  for(int i = 0; i < x; ++i)
  {
    for(int j = 0; j < y; ++j)
    {
      for(int k = 0; k < y; ++k)
        grid.push_back(make_pair(i*y + j, i*y + k));
      
      for(int k = 0; k < x; ++k)
        grid.push_back(make_pair(i*y + j, k*y + j));
    }
  }  
  return grid;
}


int main(int argc, char** argv)
{
  int grid_size = atoi(argv[1]);
  int size = grid_size * grid_size;
  
  vector<pair<int, int> > tuplelist = makeGrid(grid_size, grid_size);
  
  sort(tuplelist.begin(), tuplelist.end());
  
  vector<pair<int, int> >::iterator it = unique(tuplelist.begin(), tuplelist.end());
  tuplelist.resize(it - tuplelist.begin());
  
  vector<pair<int, int> > revlist = reverseList(tuplelist, size);
  
  sort(revlist.begin(), revlist.end());
  
  printf("MINION 3\n");
  printf("**TUPLELIST**\n");
  
  printf("PosList %d 2\n", tuplelist.size());
  for(int i = 0; i < tuplelist.size(); ++i)
    printf("%d %d \n", tuplelist[i].first + 1, tuplelist[i].second + 1);
  printf("\n");

  printf("NegList %d 2\n", revlist.size());
  for(int i = 0; i < revlist.size(); ++i)
    printf("%d %d \n", revlist[i].first + 1, revlist[i].second + 1);
  printf("\n");
  
  printf("**VARIABLES**\n");
  
  printf("DISCRETE PERM[%d] {1..%d}\n", size, size);
  
  printf("**SEARCH**\n");
  printf("PRINT [PERM]\n");
  printf("PERMUTATION [PERM]\n");
  printf("VARORDER [PERM]\n");
  printf("**CONSTRAINTS**\n");
  printf("alldiff(PERM)\n");
  
  for(int i = 0; i < size; ++i)
    for(int j = 0; j < i; ++j)
    {
      if(binary_search(tuplelist.begin(), tuplelist.end(), make_pair(i,j)))
        printf("table([PERM[%d], PERM[%d]], PosList)\n", i, j);
     // else
      //  printf("table([PERM[%d], PERM[%d]], NegList)\n", i, j);
    }
  
  printf("**EOF**\n");
}