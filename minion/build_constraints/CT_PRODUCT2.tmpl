#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_product.h"
#include "../constraints/constraint_and.h"

inline AbstractConstraint*
BuildCT_PRODUCT2(StateObj* stateObj, const vector<BoolVarRef>& vars, const vector<BoolVarRef>& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return AndCon(stateObj, vars[0], vars[1], var2[0]);
}

template<typename VarRef1, typename VarRef2>
AbstractConstraint*
BuildCT_PRODUCT2(StateObj* stateObj, const vector<VarRef1>& vars, const vector<VarRef2>& var2, ConstraintBlob&)
{ 
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new ProductConstraint<VarRef1,VarRef1,VarRef2>(stateObj, vars[0],vars[1],var2[0]); 
}

BUILD_CT(CT_PRODUCT2, 2)
