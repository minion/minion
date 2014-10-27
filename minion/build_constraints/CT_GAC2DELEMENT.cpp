#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_GAC2delement.h"

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_GAC2DELEMENT(StateObj* stateObj, const Var1& vararray, const Var2& indexarray, const Var3& v3, ConstraintBlob& b)
{ 
  return new GAC2DElementConstraint<Var1, Var2, AnyVarRef>
              (stateObj, vararray, indexarray, AnyVarRef(v3[0]), b.constants[0][0]);  
}

template<typename Var1, typename Var2>
AbstractConstraint*
BuildCT_GAC2DELEMENT(StateObj* stateObj, const Var1& vararray, const Var2& indexarray, const Var1& v3, ConstraintBlob& b)
{ 
  return new GAC2DElementConstraint<Var1, Var2, typename Var1::value_type>
              (stateObj, vararray, indexarray, v3[0], b.constants[0][0]);  
}

BUILD_CT(CT_GAC2DELEMENT, 3)
