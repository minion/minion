#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/unary/dynamic_literal.h"

template<typename VarArray1>
AbstractConstraint*
BuildCT_WATCHED_LIT(const VarArray1& _var_array_1, const ConstraintBlob& b)
{ 
  return new WatchLiteralConstraint<typename VarArray1::value_type>
    (_var_array_1[0], b.constants[0][0]); 
}

BUILD_CT(CT_WATCHED_LIT, 1)
