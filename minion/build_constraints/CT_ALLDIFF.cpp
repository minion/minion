#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_neq.h"

template<typename VarArray>
AbstractConstraint*
BuildCT_ALLDIFF(StateObj* stateObj, const VarArray& var_array, ConstraintBlob&)
{ return new NeqConstraint<VarArray>(stateObj, var_array); }

BUILD_CT(CT_ALLDIFF, 1)
