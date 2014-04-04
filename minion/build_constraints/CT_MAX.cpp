#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_min.h"

template<typename VarArray, typename VarRef>
AbstractConstraint*
BuildCT_MAX(StateObj* stateObj, const VarArray& _var_array, const vector<VarRef>& _var_ref, ConstraintBlob&)
{ return (new MinConstraint<typename NegType<VarArray>::type, typename NegType<VarRef>::type>(stateObj,
              VarNegRef(_var_array), VarNegRef(_var_ref[0]))); 
}

BUILD_CT(CT_MAX, 2)
