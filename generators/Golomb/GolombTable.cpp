//#include "../InstanceHelp.h"

#include <iostream>
#include <time.h>
#include <vector>
#include <algorithm>

using namespace std;

void outvar (int var) 
{ 
  cout << "x" << var ; 
}

vector<vector<int> > three_var_diff(int vec_length)
{
  vector<vector<int> > tuples;
  for(int i = 0; i < vec_length; ++i)
    for(int j = i+1; j < vec_length; ++j)
      for(int k = j+1; k < vec_length; ++k)
      {
	    if(k-j != j-i)
		{
		  vector<int> tuple(3);
		  tuple[0] = i;
		  tuple[1] = j;
		  tuple[2] = k;
		  
		  tuples.push_back(tuple);
		}
      }
  return tuples;
}

vector<vector<int> > four_var_diff(int vec_length)
{
  vector<vector<int> > tuples;
  
  for(int i = 0; i < vec_length; ++i)
    for(int j = i+1; j < vec_length; ++j)
	  for(int k = j+1; k < vec_length; ++k)
	    for(int z = k+1; z < vec_length; ++z)
		{
		  if(z-k != j-i)
		  {
		    vector<int> tuple(4);
			tuple[0] = i;
			tuple[1] = j;
			tuple[2] = k;
			tuple[3] = z;
			
		      tuples.push_back(tuple);
		  }
	    }
  return tuples;
}


void varinfo (int nticks, int length) 
{ 
  cout << 0 << endl ;  // booleans
  cout << 0 << endl ;  // bounds of type 1
  cout << 0 << endl ;  // bounds of type 2

  int numvars = (nticks * (nticks-1))/2 + 1 ;
  cout << numvars << endl ; // number of discrete vars
  cout << 0 << " " << length-1 << " " << numvars << endl;

  cout << 0 << endl ;  // discrete vars of type 2
}

int tickvar (int i, int nticks) 
{
  return i;
}

void heuristicinfo (int nticks) 
{
  cout << "[" ; 
  for(int i=1; i<nticks; i++) 
  { 
    outvar(tickvar(i,nticks));
    if (i < nticks-1) (cout << ",") ;
  }
  cout << "]" << endl;

  cout << "[" ; 
  for(int i=1; i<nticks; i++) 
  { 
    cout << "a" ; 
    if (i < nticks-1) (cout << ",") ;
  }
  cout << "]" << endl;
}



// We will insist that t0 = 0 
// It is more efficient in propagation to use ti for diff(ti,t0)

// the first n vars are 0 ... n-1 for the ticks 
// vars 1 .. n-1 are the differences at first level 
// vars n ...    are the differences at second level

// The difference between ti and t(i+1) is 1 + n-1 + n-2 + ... + n-i
// = 1 + i.n - (1 + 2 + .. i)
// = 1 + i.n - (i(i+1)/2)

// So the difference between ti and tj is (if j > i) 
// = (j-i) + in - (i(i+1)/2)

int diffvar (int i, int j, int nticks) 
{
  if(i == j)
  {
    cerr << "Error!" << endl;
    abort();
  }
  int first  = ( i < j ? i : j);
  int second = ( i < j ? j : i);
  if (first == 0)
    return tickvar(second, nticks);
  else 
  { return (second - first) + (first*nticks) - (first*(first+1)/2);
  };

}

void all_different (int nticks)
{
  cout << "alldiff([" ;
  for (int i=0; i < nticks ; i++)
  {
    for (int j=i+1; j < nticks ; j++) 
    {
      outvar(diffvar(i,j,nticks));
      if (i != nticks-2) cout << ",";    // only false for last pair 
    }
  }
  cout << "])" << endl;
}

void diffs_setup (int nticks)
{
  for (int i=0; i < nticks ; i++)
  {
    for (int j=i+1; j < nticks ; j++) 
    {
      int diff = diffvar(i,j,nticks);

      cout << "sumleq([" ;
      outvar(tickvar(i,nticks));
      cout << ",";
      outvar(diff);
      cout << "]," ; 
      outvar(tickvar(j,nticks));
      cout << ")" << endl;

      cout << "sumgeq([" ;
      outvar(tickvar(i,nticks));
      cout << ",";
      outvar(diff);
      cout << "]," ; 
      outvar(tickvar(j,nticks));
      cout << ")" << endl;

    }
  }
}

void symmetry_break (int nticks)
{
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
}

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

    

int main(int argc, char** argv)
{

  if(argc < 2)
  {
        cerr << "usage: GolombMinionGenerator nticks maxlength [optimising]" 
             << endl 
             << "       nticks is number of ticks"
             << endl
             << "       maxlength [default n*n] is max length of ruler"
             << endl
             << "       optimising [default 1] is 1 if optimising for shortest ruler"
             << endl 
             << "                                 0 if searching for existence of ruler at maxlength"
             << "       implied  [default 1] is   1 if using implied constraints"
             << endl 
             << "                                 0 if not"
             << endl;
        return(0);
  }

  int nticks = atoi(argv[1]);
  int maxlength = (argc < 3 ? nticks*nticks : atoi(argv[2]));

  bool optimising = (argc < 4 ? 1 : atoi(argv[3]));
  bool implied = (argc < 5 ? 1 : atoi(argv[4]));
  bool table = (argc < 6 ? 1 : atoi(argv[5]));
  
  time_t timenow; 
  time(&timenow);
  cout << "MINION 1" << endl;
  cout << "# Input file for Minion built for Version 0.2" << endl;
  cout << "#    http://sourceforge.net/projects/minion" << endl;
  cout << "# Golomb Ruler instance for input to Minion" << endl;
  cout << "#  CSPLib prob006, http://www.csplib.org" << endl;
  cout << "#     Implied Constraints from Smith/Stergiou/Walsh APES 11-1999" << endl;
  cout << "#  n = " << nticks << endl;
  cout << "#  maxlength = " << maxlength << endl;
  cout << "#  Optimising?: " << (optimising ? "yes" : "no") << endl;
  cout << "#  Implied Constraints? " << (implied ? "yes" : "no") << endl;
  cout << "#  Created: UTC " << asctime(gmtime(&timenow));
  cout << "#  Generator program written by Ian Gent" << endl;
  cout << "#  " << endl;
  cout << endl;

  varinfo(nticks, maxlength); 

  heuristicinfo(nticks);

  cout << 0 << endl; // no vectors
  cout << 1 << endl ; // 1 matrix, for printing 
  cout << "[[" ;
  outvar( tickvar(0,nticks) );
  for(int i=1; i < nticks ; i++) 
  { 
    cout << "," ;
    outvar( tickvar(i,nticks) );
  }
  for(int i=0; i < nticks-1 ; i++)
  {
    cout << "]," << endl << " [" ;
    outvar( diffvar(i,i+1,nticks) );
    for(int j=i+2; j < nticks ; j++)
    { 
      cout << "," ;
      outvar( diffvar(i,j,nticks) ) ;
    }
  }
  cout << "]]" << endl;
  cout << 0 << endl ; // no tensors etc

  cout << "objective " ;
  if (optimising)
  {
    cout << "minimising " ;
    outvar(tickvar(nticks-1,nticks));
    cout << endl ; 
  }
  else
  { 
    cout << "none" << endl ;
  };
  cout << "print m0" << endl;
  cout << "eq(";                // t[0] = 0
  outvar(tickvar(0,nticks));
  cout << ",0)" << endl;

  all_different(nticks);

  symmetry_break(nticks);

  diffs_setup(nticks);

  if (implied) implied_constraints(nticks);

vector<vector<int> > var3 = three_var_diff(maxlength);
  vector<vector<int> > var4 = four_var_diff(maxlength);
  
  if(table)
  {
  for(int i = 0; i < nticks; ++i)
    for(int j = i + 1; j < nticks; ++j)
	  for(int k = j + 1; k < nticks; ++k)
	  {
	    printf("table([x%d,x%d,x%d],{", i, j, k);
		
		for(int z = 0; z < var3.size()-1; z++)
		  printf("<%d,%d,%d>,\n", var3[z][0], var3[z][1], var3[z][2]);
		
		printf("<%d,%d,%d>\n", var3[0][var3.size()-1],var3[0][var3.size()-1],var3[0][var3.size()-1]);

		printf("})\n");
	  }

  for(int i = 0; i < nticks; ++i)
    for(int j = i + 1; j < nticks; ++j)
	  for(int k = j + 1; k < nticks; ++k)
	  for(int l = k + 1; l < nticks; ++l)
	  {
	    printf("table([x%d,x%d,x%d,x%d],{", i, j, k, l);
		
		for(int z = 0; z < var4.size()-1; z++)
		  printf("<%d,%d,%d,%d>,\n", var4[z][0], var4[z][1], var4[z][2],var4[z][3]);
		printf("<%d,%d,%d,%d>\n", var4[0][var4.size()-1],var4[0][var4.size()-1],
								  var4[0][var4.size()-1],var4[0][var4.size()-1]);
		printf("})\n");
	  }
  }

}
