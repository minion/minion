/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/



#include "minion.h"
#include "test_functions.h"
#define VAR_COUNT 5



int main(void)
{
  vector<LRangeVarRef> vars;
  vector<LRangeVarRef> var1(VAR_COUNT);
  vector<LRangeVarRef> var2(VAR_COUNT);
  for(int i=0;i<VAR_COUNT*2;i++)
    vars.push_back(rangevar_container.get_new_var(0,5));
  
  for(int i=0;i<VAR_COUNT;i++)
  {
    var1[i]=vars[i];
    var2[i]=vars[i+VAR_COUNT];
  }
  
  Constraint* c = OccEqualCon(var1, runtime_val(1), runtime_val(1));
  Constraint* d = OccEqualCon(var2, runtime_val(1), runtime_val(1));
  Constraint* e = c->get_table_constraint();
  Controller::add_constraint(d);
  Controller::add_constraint(e);
  Controller::lock();
  test_equal(var1,var2);
}



