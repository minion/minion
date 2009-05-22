#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>

using namespace std;

class MinionSportsInstanceGenerator {
  void generateVars(int n) ;
  void generateVarOrder(int n) ;
  void generateValOrder(int n) ;
  void generateMatrices(int n) ;
  void generateConstraints(int n) ;
 public:
  void generate(int n) ;
};
