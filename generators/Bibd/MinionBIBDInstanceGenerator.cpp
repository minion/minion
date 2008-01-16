// MinionBIBDInstanceGenerator.cpp

#include "../InstanceHelp.h"
#include "MinionBIBDInstanceGenerator.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Generate
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generate(int v, int b, int r, int k,
  int l) {
  generateVars(v, b) ;
  generateVarOrder(v, b) ;
  generateValOrder(v, b) ;
  generateMatrices(v, b) ;
  cout << "objective none" << endl ;
  cout << "print m0" << endl;
  generateConstraints(v, b, r, k, l) ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVars
//b x v decision vars
//b vars for every scalar prod: n(n+1)/2 (where n is v-1).
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateVars(int v, int b) {
  cout << ((b*v) + (b*(v-1)*(v))/2) << endl ;
  cout << endl << 0 << endl << 0 << endl  << 0 << endl << 0 << endl ;   // other vars
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVarOrder
// row-wise 
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateVarOrder(int v, int b) {
  int i1, i2, var = 0 ;
  cout << "[" ;
  for (i1 = 0; i1 < v; i1++) {
    for (i2 = 0; i2 < b; i2++) {
      cout << "x" << var++ ;
      if ((i1 < v-1) || (i2 < b-1)) cout << "," ;
    }
    cout << endl ;
  }
  cout << "]" << endl; 
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateValOrder
// Ascending
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateValOrder(int v, int b) {
  int i1, i2 ;
  cout << "[" ;
  for (i1 = 0; i1 < v; i1++) {
    for (i2 = 0; i2 < b; i2++) {
      cout << "a" ;
      if ((i1 < v-1) || (i2 < b-1)) cout << "," ;
    }
    cout << endl ;
  }
  cout << "]" << endl; 
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateMatrices
// 1 vector per scalar product
// 1 matrix of decision vars.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateMatrices(int v, int b) {
  int i1, i2, noVectors = ((v-1)*(v))/2, var = b*v ;
  cout << noVectors << endl ;
  for (i1 = 0; i1 < noVectors; i1++) {
    cout << "[" ;
    for (i2 = 0; i2 < b; i2++) {
      cout << "x" << var++ ;
      if (i2 < b-1) cout << ", " ;
    }
    cout << "]" << endl ;
  }
  //2d matrix of decision vars.
  var = 0 ;
  cout << endl << 1 << endl << "[" ;
  for (i1 = 0; i1 < v; i1++) {
    cout << "[" ;
    for (i2 = 0; i2 < b; i2++) {
      cout << "x" << var++ ;
      if (i2 < b-1) cout << ", " ;
    }
    cout << "]" ;
    if (i1 < v-1) cout << "," << endl ;
  }
  cout << "]" << endl ;
  //There are no tensors
  cout << endl << 0 << endl << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//generateConstraints
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateConstraints(int v,
  int b, int r, int k, int l) {
  int i1, i2, i3 ;
  // row sums
  for (i1 = 0; i1 < v; i1++)
  {
    cout << "sumleq(row(m0, " << i1 << "), " << r << ")" << endl ;
    cout << "sumgeq(row(m0, " << i1 << "), " << r << ")" << endl ;
  }
  // col sums
  for (i1 = 0; i1 < b; i1++)
  {
    cout << "sumleq(col(m0, " << i1 << "), " << k << ")" << endl ;
    cout << "sumgeq(col(m0, " << i1 << "), " << k << ")" << endl ;
}
  cout << endl ;
  // scalar products
  int row1var = 0, row2var = b, prodvar = b*v, vecno = 0 ;
  for (i1 = 0; i1 < v; i1++) {
    for (i2 = i1+1; i2 < v; i2++) {
      for (i3 = 0; i3 < b; i3++) {
        cout << "product(x" << row1var++ << ", x" << row2var++ <<
                ", x" << prodvar++ << ")" << endl ;
      }
      cout << "sumleq(v" << vecno << ", " << l << ")" << endl << endl ;
      cout << "sumgeq(v" << vecno++ << ", " << l << ")" << endl << endl ;
      
      if (i2 < v-1) row1var -= b ;
    }
    if (i1 < v-1) row2var = row1var + b ;
  }
  //symmetry breaking: rows
  for (i1 = 0; i1 < v-1; i1++)
    cout << "lexleq(row(m0, " << i1 << "), row(m0, " << (i1+1)
         << "))" << endl ;
  cout << endl ;
  //symmetry breaking: cols
  for (i1 = 0; i1 < b-1; i1++)
    cout << "lexleq(col(m0, " << i1 << "), col(m0, " << (i1+1)
         << "))" << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
// Params: v, b, r, k, l
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int main(int argc, char* argv[]) {
  MinionBIBDInstanceGenerator generator ;
  if (argc != 6)
    cout << "params: v, b, r, k, lambda" << endl ;
  else
  {
    // Check is a Minion file, and give version number of input format
	cout << "MINION 1" << endl;
    instance dummy = instance();
    cout << dummy.header();       // dummy only created to print header
    
    cout << "# BIBD Instance" << endl 
         << "# CSPLib prob028, www.csplib.org" << endl
         << "# Parameters: v, b, r, k, lambda = "
         << argv[1] << ", " << argv[2] << ", " << argv[3] << ", " 
         << argv[4] << ", " << argv[5] << endl;

    generator.generate(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                       atoi(argv[4]), atoi(argv[5])) ;
  return 0 ;
  }
}
