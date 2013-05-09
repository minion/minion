#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_min.h"

template<typename VarArray, typename VarRef>
AbstractConstraint*
BuildCT_MIN(StateObj* stateObj, const VarArray& _var_array, const vector<VarRef>& _var_ref, ConstraintBlob&)
{ return (new MinConstraint<VarArray,VarRef>(stateObj, _var_array, _var_ref[0])); }

BUILD_CT(CT_MIN, 2)
