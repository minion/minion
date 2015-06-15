#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_nvalue.h"

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BuildCT_LEQNVALUE(const VarArray& _var_array, const vector<VarSum>& _var_sum, ConstraintBlob&)
{ 
    return new LessEqualNvalueConstraint<VarArray, VarSum>(_var_array, _var_sum[0]); 
}

BUILD_CT(CT_LEQNVALUE, 2)
