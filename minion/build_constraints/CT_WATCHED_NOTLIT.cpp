#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/unary/dynamic_notliteral.h"

AbstractConstraint*
BuildCT_WATCHED_NOTLIT(const vector<BoolVarRef>& vec, const ConstraintBlob& b)
{ return new WatchNotLiteralBoolConstraint(vec[0], b.constants[0][0]); }

template<typename VarArray1>
AbstractConstraint*
BuildCT_WATCHED_NOTLIT(const VarArray1& _var_array_1, const ConstraintBlob& b)
{ 
  return new WatchNotLiteralConstraint<typename VarArray1::value_type>
    (_var_array_1[0], b.constants[0][0]); 
}

BUILD_CT(CT_WATCHED_NOTLIT, 1)
