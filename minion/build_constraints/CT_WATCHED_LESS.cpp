#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/dynamic_less.h"

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
BuildCT_WATCHED_LESS(StateObj* stateObj, const VarArray1& _var_array_1, const VarArray2& _var_array_2, ConstraintBlob&)
{ 
  return new WatchLessConstraint<typename VarArray1::value_type, typename VarArray2::value_type>
    (stateObj, _var_array_1[0], _var_array_2[0]); 
}

BUILD_CT(CT_WATCHED_LESS, 2)
