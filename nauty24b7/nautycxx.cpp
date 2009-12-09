#include "nausparse.h"    /* which includes nauty.h */
#include <vector>
#include <set>
#include <cassert>

// This following file provides a C++ wrapper around nauty,
// Developed by Christopher Jefferson, (c) 2009
// You are welcome to use this code in any way you like.

std::vector<std::vector<int> > global_perm_stored_list;

void getauto(int,permutation* p,int*,int,int,int n)
{ global_perm_stored_list.push_back(std::vector<int>(p, p + n)); }

std::vector<std::vector<int> > build_graph(std::vector<std::set<int> > graph, const std::vector<std::set<int> >& partition)
{
  global_perm_stored_list.clear();
  DYNALLSTAT(int,lab,lab_sz);
  DYNALLSTAT(int,ptn,ptn_sz);
  DYNALLSTAT(int,orbits,orbits_sz);
  DYNALLSTAT(setword,workspace,workspace_sz);
  static DEFAULTOPTIONS_SPARSEGRAPH(options);
  statsblk stats;
  sparsegraph sg;   /* Declare sparse graph structure */

  int n = graph.size();
  int m = (n + WORDSIZE - 1) / WORDSIZE;
  
  int edge_count = 0;

   for(int i = 0; i < graph.size(); ++i)
   {
     for(std::set<int>::const_iterator it = graph[i].begin(); it != graph[i].end(); ++it)
       graph[*it].insert(i);
   }

   for(int i = 0; i < graph.size(); ++i)
     edge_count += graph[i].size();
     
//  options.writeautoms = TRUE;
  options.userautomproc = &getauto;
  options.defaultptn = FALSE;

  SG_INIT(sg);

  // Number of vertices
  nauty_check(WORDSIZE, m, n, NAUTYVERSIONID);

  DYNALLOC1(int,lab,lab_sz,n,"malloc");
  DYNALLOC1(int,ptn,ptn_sz,n,"malloc");
  DYNALLOC1(int,orbits,orbits_sz,n,"malloc");
  DYNALLOC1(setword,workspace,workspace_sz,50*m,"malloc");

 

  SG_ALLOC(sg, n, edge_count, "malloc");

  sg.nv = n;
  sg.nde = edge_count;

  int current_pos = 0;
  for(int i = 0; i < n; ++i)
  {
    sg.v[i] = current_pos;
    sg.d[i] = graph[i].size();
    for(std::set<int>::const_iterator it = graph[i].begin(); it != graph[i].end(); ++it)
    {
      sg.e[current_pos] = *it;
      current_pos++;
    }
  }
  
  // Now for partitioning
  current_pos = 0;
  for(int i = 0; i < partition.size(); ++i)
  {
    for(std::set<int>::const_iterator it = partition[i].begin(); it != partition[i].end(); ++it)
    {
      lab[current_pos] = *it;
      ptn[current_pos] = 1;
      current_pos++;
    }
    ptn[current_pos - 1] = 0;
  }
  
  assert(current_pos == n);
  
               
  nauty(reinterpret_cast< ::graph*>(&sg),lab,ptn,NULL,orbits,&options,&stats,
                                        workspace,50*m,m,n,NULL);
 
  return global_perm_stored_list;
}

/*
int main(void)
{
  std::vector<std::set<int> > verts;
  
  verts.push_back(std::set<int>());
  for(int i = 1; i < 99; ++i)
  {
    std::set<int> s;
    s.insert(i-1);
    s.insert(i+1);
    verts.push_back(s);
  }
  verts.push_back(std::set<int>());
  

  std::vector<std::set<int> > partitions;
  
  std::set<int> s;
  
  for(int i = 0; i < 100; ++i)
    s.insert(i);

  partitions.push_back(s);
 
  std::vector<std::vector<int> > perms = build_graph(verts, partitions);
  
  for(int i = 0; i < perms.size(); ++i)
   {
     for(int j = 0; j < perms[i].size(); ++j)
       printf("%d,", perms[i][j]);
     printf("\n");
  }
}
*/
