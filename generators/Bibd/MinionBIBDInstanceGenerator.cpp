// MinionBIBDInstanceGenerator.cpp

#include "../InstanceHelp.h"
#include "MinionBIBDInstanceGenerator.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Generate
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generate(int v, int b, int r, int k,
  int l, bool doLex) {
  generateVars(v, b) ;
  generateVarOrder(v, b) ;
  cout << "PRINT [MATRIX]" << endl;
  generateConstraints(v, b, r, k, l, doLex) ;
  printf("**EOF**\n");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVars
//b x v decision vars
//b vars for every scalar prod: n(n+1)/2 (where n is v-1).
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateVars(int v, int b) {
  printf("**VARIABLES**\n");
  printf("BOOL MATRIX[%d,%d]\n", v, b);
  printf("BOOL DIFFS[%d, %d]\n", (v*(v-1))/2, b);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVarOrder
// row-wise 
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateVarOrder(int v, int b) {
  printf("**SEARCH**\n");
  printf("VARORDER [MATRIX, DIFFS]\n");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//generateConstraints
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionBIBDInstanceGenerator::generateConstraints(int v,
  int b, int r, int k, int l, bool doLex) {
    
    printf("**CONSTRAINTS**\n");
  int i1, i2, i3 ;
  // row sums
  for (i1 = 0; i1 < v; i1++)
  {
    cout << "sumleq(MATRIX[" << i1 << ",_], " << r << ")" << endl ;
    cout << "sumgeq(MATRIX[" << i1 << ",_], " << r << ")" << endl ;
  }
  // col sums
  for (i1 = 0; i1 < b; i1++)
  {
    cout << "sumleq(MATRIX[_," << i1 << "], " << k << ")" << endl ;
    cout << "sumgeq(MATRIX[_," << i1 << "], " << k << ")" << endl ;
}
  cout << endl ;
  // scalar products
  int prodvec = 0 ;
  for (i1 = 0; i1 < v; i1++) {
    for (i2 = i1+1; i2 < v; i2++) {
      for (i3 = 0; i3 < b; i3++) {
        printf("product(MATRIX[%d,%d], MATRIX[%d,%d], DIFFS[%d,%d])\n", i1,i3 ,i2,i3, prodvec,i3);
      }
      printf("sumleq(DIFFS[%d,_], %d)\n", prodvec, l);
      printf("sumgeq(DIFFS[%d,_], %d)\n", prodvec, l);
          prodvec++;
    }
  }
  
  if(doLex)
  {
  //symmetry breaking: rows
  for (i1 = 0; i1 < v-1; i1++)
    printf("lexleq(MATRIX[%d,_], MATRIX[%d,_])\n", i1, i1+1);
  //symmetry breaking: cols
  for (i1 = 0; i1 < b-1; i1++)
    printf("lexleq(MATRIX[_,%d], MATRIX[_,%d])\n", i1, i1+1);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
// Params: v, b, r, k, l
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int main(int argc, char* argv[]) {
  MinionBIBDInstanceGenerator generator ;
  if (argc != 7)
    cout << "params: v, b, r, k, lambda, usedoublelex (0/1)" << endl ;
  else
  {
    // Check is a Minion file, and give version number of input format
	cout << "MINION 3" << endl;
    instance dummy = instance();
    cout << dummy.header();       // dummy only created to print header
    
    cout << "# BIBD Instance" << endl 
         << "# CSPLib prob028, www.csplib.org" << endl
         << "# Parameters: v, b, r, k, lambda, usedoublelex = "
         << argv[1] << ", " << argv[2] << ", " << argv[3] << ", " 
         << argv[4] << ", " << argv[5] << argv[6] << endl;

    generator.generate(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                       atoi(argv[4]), atoi(argv[5]), atoi(argv[6])) ;
                       
                       
  return 0 ;
  }
}
