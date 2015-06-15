#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_element.h"

template<typename Var1, typename Var2>
AbstractConstraint*
BuildCT_ELEMENT(const Var1& vararray, const Var2& v1, const Var1& v2, ConstraintBlob&)
{ 
  return new ElementConstraint<Var1, typename Var2::value_type, typename Var1::value_type>
              (vararray, v1[0], v2[0]);  
}

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_ELEMENT(Var1 vararray, const Var2& v1, const Var3& v2, ConstraintBlob&)
{ 
  return new ElementConstraint<Var1, typename Var2::value_type, AnyVarRef>
              (vararray, v1[0], AnyVarRef(v2[0]));  
}

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_ELEMENT_ONE(const Var1& vararray, const Var2& v1, const Var3& v2, ConstraintBlob& b)
{ 
  typedef typename ShiftType<typename Var2::value_type, compiletime_val<SysInt, -1> >::type ShiftVal;
  vector<ShiftVal> replace_v1;
  replace_v1.push_back(ShiftVarRef(v1[0], compiletime_val<SysInt, -1>()));
  return BuildCT_ELEMENT(vararray, replace_v1, v2, b);
}

BUILD_CT(CT_ELEMENT_ONE, 3)
