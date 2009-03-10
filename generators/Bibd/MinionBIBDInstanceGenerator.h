#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>

using namespace std;

class MinionBIBDInstanceGenerator {
  void generateVars(int v, int b) ;
  void generateVarOrder(int v, int b) ;
  void generateValOrder(int v, int b) ;
  void generateMatrices(int v, int b) ;
  void generateConstraints(int v, int b, int r, int k, int l, bool) ;
 public:
  void generate(int v, int b, int r, int k, int l, bool) ;
};
