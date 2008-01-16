//#include "../InstanceHelp.h"

#include <iostream>
#include <vector>
#include <time.h>

using namespace std;

int numdiffs;
int numvars;
int numsearchvars;

vector<int> searchvars;

void outvar (int var) 
{ 
  cout << "x" << var ; 
}


int varinfo (int k, int p)
{ 
  cout << 0 << endl ;  // booleans
  cout << 0 << endl ;  // bounds of type 1
  cout << 0 << endl ;  // bounds of type 2

  numdiffs = p*(k*(k-1))/2 + (p-1)*k;
  numsearchvars = k*p + numdiffs - k; 
  int numvars = numsearchvars + 2*numdiffs;    // -k for nodevars used for diffs

  cout << numvars << endl ; // number of discrete vars
  cout << 0 << " " << 0 << " " << 1 << endl ;   // i.e. x0=0
  cout << 1 << " " << numdiffs << " " << numvars-1-(2*numdiffs) << endl;
  cout << 0 << " " << numdiffs << " " << (2*numdiffs) << endl;

  cout << 0 << endl ;  // discrete vars of type 2

  return numvars;
}

int nodevar (int i, int clique, int k, int p) 
{
  return clique*k + i;
}

// Returns -1 to mean that no such difference var exists
//
// We will insist that x0 = 0 
// It is more efficient in propagation to use ti for diff(ti,t0)

// the first n vars are 0 ... k*p-1 for the ticks 
// then we have k*(p-1) - 1 for differences along the paths
// then the remainder for differences in a clique

int diffvar (int i, int j, int k, int p) 
{

  if ( (i >= k*p) or 
       (j >= k*p) or 
       (i == j) or 
       ((i/k) > (j/k) +1) or
       ((j/k) > (i/k) +1) or 
       ((i/k) != (j/k) and  (i%k != j%k) ) 
     )
  {
    //cerr << i << " " << j << " " <<  endl;
    return -1;
  }

  int result;
  int first  = ( i < j ? i : j);
  int second = ( i < j ? j : i);
  int clique = first / k;

  if (first == 0)
    result= (second);
  else if ( (first/k) != (second/k) )            //
  {
    result= (k*p) + (first) - 1; 
  }
  else if ( (first/k) == (second/k) ) 
  { int newi = first%k;
    int newj = second%k;
    int count = 0;
    for( int loop = 0 ; loop < newi ; ++loop) count += (k-1-loop);

    result= (k*p) + k*(p-2) + 
           (k*(k-1)/2) * (first/k) + 
           count + (newj-newi) - 1;
  }
  else 
  { 
     cerr << "Error: k= " << k << "i= " << i << "j = " << j << endl;
  }

  // cerr << i << " " << j << " " << result << endl;

  return result;

}


void heuristicinfo (int k, int p) 
{
  for(int clique = 0; clique < p; clique++) 
  {
    for(int i=0; i < k; i++)
    {
      int thisnode = nodevar(i,clique,k,p);
      searchvars.push_back(thisnode);
      if ((clique > 0) and (i>0)) 
      {
        int arraysize = searchvars.size();
        for(int j=0; j<arraysize; ++j) 
        {
          int diff = diffvar(thisnode,searchvars[j],k,p);
          if (diff > -1)
          {
            searchvars.push_back(diff+numdiffs);
            searchvars.push_back(diff+2*numdiffs);
            searchvars.push_back(diff);
          }
        }
      }
    }
  }

  numsearchvars = searchvars.size();

  cout << "[" ; 
  for(int j=0; j< numsearchvars; ++j) 
  { 
    outvar(searchvars[j]);
    if (j < numsearchvars-1) (cout << ",") ;
  }
  cout << "]" << endl;

  cout << "[" ; 
  for(int i=0; i< numsearchvars; i++) 
  { 
    cout << "a" ; 
    if (i < numsearchvars-1) (cout << ",") ;
  }
  cout << "]" << endl;
}




void all_different (int k, int p)
{
  cout << "alldiff([" ;                // node vars must all be different
  for (int i=0; i < k*p ; i++)
  {
      outvar(i);                        // note lack of information hiding
      if (i < k*p-1) cout << ",";    
  }
  cout << "])" << endl;

  cout << "alldiff([" ;                 // all diffs must be different
  for (int clique = 0; clique < p ; clique++)
  {
    for (int i=0; i < k ; i++) 
    {
      if (clique < p-1) 
      {
        outvar(diffvar(nodevar(i,clique,k,p),nodevar(i,clique+1,k,p),k,p));
        cout << ",";
      };
      for (int j=i+1; j < k; j++)
      {
        outvar(diffvar(nodevar(i,clique,k,p),nodevar(j,clique,k,p),k,p));
        if ((clique != p-1) or (i < k-2)) cout << ",";
                // only false for last pair 
      }
    }
  }
  cout << "])" << endl;
}

void onediff_print (int var1, int var2, int k, int p) 
{
        int diff = diffvar(var1,var2,k,p);
        int newmin = diff + numdiffs;
        int newmax = diff + 2*numdiffs;

        cout << "sumleq([" ;
        outvar(newmin);
        cout << ",";
        outvar(diff);
        cout << "]," ; 
        outvar(newmax);
        cout << ")" << endl;
      
        cout << "sumgeq([" ;
        outvar(newmin);
        cout << ",";
        outvar(diff);
        cout << "]," ; 
        outvar(newmax);
        cout << ")" << endl;

        cout << "min([" ;
        outvar(var1);
        cout << "," ;
        outvar(var2);
        cout << "]," ;
        outvar(newmin);
        cout << ")" << endl;
        
        cout << "max([" ;
        outvar(var1);
        cout << "," ;
        outvar(var2);
        cout << "]," ;
        outvar(newmax);
        cout << ")" << endl;
        
}

void diffs_setup (int k, int p)
{
  for (int clique = 0 ; clique < p ; ++clique) 
  {
    for (int i=0; i < k ; i++)
    {
      if (clique>0 or i>0)
      {
        if (clique < p-1) 
        {
          onediff_print(nodevar(i,clique,k,p),nodevar(i,clique+1,k,p),k,p);
        }
        for (int j=i+1; j < k ; j++) 
        {
          onediff_print(nodevar(i,clique,k,p),nodevar(j,clique,k,p),k,p);
        }
      }
    }
  }
}

void symmetry_break (int k, int p, int q)
{


  // disallow q and q-1 from domains not adjacent to 0
  //

  cout << "occurrence([" ;
  outvar(1);
  for (int i = 2; i < k+1 ; i++) 
  {
    cout << ",";
    outvar(i);
  }
  cout << "]," << q << "," << "1)" << endl;

  cout << "occurrence([" ;
  outvar(1);
  for (int i = 2; i < k+1 ; i++) 
  {
    cout << ",";
    outvar(i);
  }
  cout << "]," << q-1 << "," << "1)" << endl;
    
  for(int i = 1; i<k-1 ; ++i) 
  {
    cout << "ineq(" ;
    outvar(nodevar(i,0,k,p));
    cout << "," ;
    outvar(nodevar(i+1,0,k,p));
    cout << ",-1)" << endl;
  }
}
    

  //
  /* 
  // ticks are in strictly increasing order
  for (int i=0;i<nticks-1;i++)
  {
     cout << "ineq(" ;
     outvar(i);
     cout << ",";
     outvar(i+1);
     cout << ",-1)" << endl;
  }
  // first difference is less than last difference
  cout << "ineq(" ;
  outvar(diffvar(0,1,nticks));
  cout << ",";
  outvar(diffvar(nticks-2,nticks-1,nticks));
  cout << ",-1)" << endl;
  */

/*
 *
void implied_constraints (int nticks)
{
  // From Smith/Stergiou/Walsh
  // d(i,j) >= (j-i)(j-i+1)/2
  // d(i,j) <= x_n - (n-1-j+i)(n-j+i)/2

  for (int i=0; i < nticks ; i++)
  {
    for (int j=i+1; j < nticks ; j++) 
    {
      int diff = diffvar(i,j,nticks);

  // d(i,j) >= (j-i)(j-i+1)/2
      // Below is a Hack.  
      // We want A >= constant, 
      // Achieve by 0 <= A - constant
      // And achieve 0 by x0 which we have set to 0 elsewhere
      
      cout << "ineq(" ;
      outvar(tickvar(0,nticks));        
      cout << ",";
      outvar(diff);
      cout << ",-" << ((j-i)*(j-i+1) )/2 << ")" << endl;

  // d(i,j) <= x_n - (n-1-j+i)(n-j+i)/2

      cout << "ineq(" ;
      outvar(diff);
      cout << ",";
      outvar(tickvar(nticks-1,nticks));        
      cout << ",-" << ((nticks-1-j+i)*(nticks-j+i))/2 << ")" << endl;

    }
  }
}

*/
    

int main(int argc, char** argv)
{

  if(argc < 3 or atoi(argv[2]) < 2)
  {
        cerr << "usage: GolombMinionGenerator nticks maxlength [optimising]" 
             << endl 
             << "       k is number in clique"
             << endl
             << "       p is number of cliques (p > 1)"
             << endl;
        return(0);
  }

  int k = atoi(argv[1]);
  int p = atoi(argv[2]);


  time_t timenow; 
  time(&timenow);
  cout << "MINION 1" << endl;
  cout << "# Input file for Minion built for Version 0.2" << endl;
  cout << "#    http://sourceforge.net/projects/minion" << endl;
  cout << "# Graceful Graph instance for input to Minion" << endl;
  cout << "#  k = " << k << endl;
  cout << "#  p = " << p << endl;
  cout << "#  Created: UTC " << asctime(gmtime(&timenow));
  cout << "#  Generator program written by Ian Gent" << endl;
  cout << "#  " << endl;
  cout << endl;

  numvars = varinfo(k, p); 


  heuristicinfo(k,p);

  cout << 0 << endl; // no vectors
  cout << 1 << endl ; // 1 matrix, for printing 
  cout << "[" ;
  for(int clique = 0 ; clique < p ; clique++) 
  {  
    cout << "[" ;
    outvar( nodevar(0,clique,k,p) );
    for(int i=1; i < k ; i++) 
    { 
      cout << "," ;
      outvar( nodevar(i,clique,k,p) );
    }
    cout << "]" ;
    cout << "," << endl;
  }
  cout << "[" ;
  for(int j=0; j< numsearchvars; ++j) 
  { 
    outvar(searchvars[j]);
    if (j < numsearchvars-1) (cout << ",") ;
  }
  cout << "]]" << endl;

  cout << 0 << endl ; // no tensors etc

  cout << "objective none" << endl ;

  cout << "print m0" << endl;
  cout << "eq(";                // t[0] = 0
  outvar(0);
  cout << ",0)" << endl;

  all_different(k,p);

  symmetry_break(k,p,numdiffs);

  diffs_setup(k,p);

  // if (implied) implied_constraints(nticks);

}
