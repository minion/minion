// MinionSportsInstanceGenerator.cpp

#include "../InstanceHelp.h"
#include "MinionSportsInstanceGenerator.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Generate
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionSportsInstanceGenerator::generate(int n) {
  generateVars(n) ;
  generateVarOrder(n) ;
  generateValOrder(n) ;
  generateMatrices(n) ;
  cout << "objective none" << endl << endl ;
  cout << "print m0" << endl << endl ;
  generateConstraints(n) ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVars
//Schedule mx:
//n/2 periods (cols) n-1 weeks (rows) a pair in each: n^2 - n
//Game mx: 1/2 * (n^2 - n)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionSportsInstanceGenerator::generateVars(int n) {
  // no 0/1 vars.
  cout << 0 << endl << endl ;
  // no Bounds vars.
  cout << 0 << endl << endl ;
  // no Sparse Bounds vars.
  cout << 0 << endl << endl ;
  // (n^2-n) + 1/2 * (n^2 - n) Discrete vars.
  cout << ((n*n-n) + ((n*n-n) / 2)) << endl ;
  cout << 0 << " " << (n-1) << " " << (n*n-n) << endl ;
  cout << 0 << " " << (n/2*(n-1))-1 << " " << (n/2*(n-1)) << endl << endl ;
  // no Sparse Discrete vars.
  cout << 0 << endl << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVarOrder
// Game array, row-wise.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionSportsInstanceGenerator::generateVarOrder(int n) {
  cout << "[" ;
  for (int i1 = n*n-n; i1 < n*n-n+(n/2)*(n-1); i1++) {
    cout << "x" << i1 ;
    if (i1 < n*n-n+(n/2)*(n-1)-1) cout << "," ;
  }
  cout << "]" << endl << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateValOrder
// Ascending
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionSportsInstanceGenerator::generateValOrder(int n) {
  cout << "[" ;
  for (int i1 = 0; i1 < (n/2*(n-1)); i1++) {
    cout << "a" ;
    if (i1 < (n/2*(n-1))-1) cout << "," ;
  }
  cout << "]" << endl << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// generateMatrices
// 1 3d matrix n/2 x (n-1) x 2 for schedule
// 1 2d matrix n/2 x (n-1) for game array.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionSportsInstanceGenerator::generateMatrices(int n) {
  int i1, i2, i3, var ;
  // Number of 1d matrices
  cout << 0 << endl << endl ; 
  // Number of 2d matrices
  cout << 1 << endl ;
  var = n*n - n ;
  cout << "[" ;
  for (i1 = 0; i1 < n-1; i1++) {
    cout << "[" ;
    for (i2 = 0; i2 < n/2; i2++) {
      cout << "x" << var++ ;
      if (i2 < n/2-1) cout << ", " ;
    }
    cout << "]" ;
    if (i1 < n-2) cout << "," << endl ;
  }
  cout << "]" << endl << endl  ;
  // Number of 3d matrices
  cout << 1 << endl ;
  var = 0 ;
  cout << "[" ;
  for (i1 = 0; i1 < 2; i1++) {
    cout << "[" ;
    for (i2 = 0; i2 < n-1; i2++) {
      cout << "[" ;
      for (i3 = 0; i3 < n/2; i3++) {
        cout << "x" << var++ ;
        if (i3 < n/2-1) cout << ", " ;
      }
      cout << "]" ;
      if (i2 < n-2) cout << "," << endl ;
    }
    cout << "]" ;
    if (i1 == 0) cout << "," << endl ;
  }
  cout << "]" << endl << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// generateConstraints
// AllDiff on rows representing a week.
// AllDiff on columns representing a period.
// AllDiff on game matrix
// Break all symm on game matrix: top-left less than all. Order row1,
//  Order col1.
// Table cts for channelling
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionSportsInstanceGenerator::generateConstraints(int n) {
  int i1, i2, i3, i4, var ;
  // AllDiff on each row representing a week.
  var = 0 ;
  for (i1 = 0; i1 < n-1; i1++) {                            // Week loop
    cout << "alldiff([" ;
    for (i2 = var; i2 < var+n/2; i2++)
      cout << "x" << i2 << ", " ;
    for (i2 = var + ((n-1)*n/2) ; i2 < var + ((n-1)*n/2) + n/2; i2++) {
      cout << "x" << i2  ;
      if (i2 < var + ((n-1)*n/2) + n/2 - 1) cout << ", " ;
    } 
    cout << "])" << endl ;
    var += n/2 ;
  }
  cout << endl ;
  // AllDiff on each col representing a period..
  var = 0 ;
  for (i1 = 0; i1 < n/2; i1++) {                         // Period loop
    cout << "alldiff([" ;
    for (i2 = var; i2 < var+(n/2)*(n-1); i2+=n/2)
      cout << "x" << i2 << ", " ;
    for (i2 = var+ (n/2*(n-1)); i2 < var + (n*(n-1)); i2+=n/2) {
      cout << "x" << i2 ;
      if (i2 < var + (n*(n-1)) - n/2) cout << ", " ;
    }
    cout << "])" << endl ;
    var++ ;
  }
  cout << endl ;
  // AllDiff on game matrix
  cout << "alldiff(m0)" << endl << endl ;
  
  // Channelling
  for (i1 = 0; i1 < n-1; i1++)                             // Week loop
    for (i2 = 0; i2 < n/2; i2++) {                       // Period loop
      cout << "table([x" << (i1*n/2)+i2 << ", x" << ((i1*n/2)+i2)+(n/2*(n-1)) 
	  << ", x" << (i1*n/2)+i2+n*n-n << "] ," << endl ;
      cout << "{" << endl ;
      int game = 0 ;
	  bool first_pass = true;
      for (i3 = 0; i3 < n; i3++)
        for (i4 = i3+1; i4 < n; i4++)
		{
		  if(first_pass)
		    first_pass = false;
		  else
		    cout << ",";
          cout << "<" << i3 << ", " << i4 << ", " << game++ << ">" << endl ;
		}
		  cout << "} )" << endl ;
    }
	 
  
  // Break all symm on game matrix: top-left less than all.
  for (i1 = n*n-n+1; i1 < (n*n-n) + ((n*n-n) / 2); i1++)
    cout << "ineq(x" << (n*n-n) << ", x" << i1 << ", 1)" << endl ;
  cout << endl ;
  // Break all symm on game matrix: order top row
  for (i1 = n*n-n; i1 < n*n-n+n/2-1; i1++)
    cout << "ineq(x" << i1 << ", x" << i1+1 << ", 1)" << endl ;
  cout << endl ;
  // Break all symm on game matrix: order left col
  for (i1 = n*n-n; i1 < n*n-n+(n/2)*(n-2); i1+= n/2)
    cout << "ineq(x" << i1 << ", x" << i1+n/2 << ", 1)" << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
// Params: n
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int main(int argc, char* argv[]) {
  MinionSportsInstanceGenerator generator ;
  if (argc != 2)
    cout << "params: n" << endl ;
  else
  {
    cout << "MINION 1" << endl;
    instance dummy = instance() ;       // only created for next line
    cout << dummy.header() ;
    generator.generate(atoi(argv[1])) ;
  }
  return 0 ;
}
