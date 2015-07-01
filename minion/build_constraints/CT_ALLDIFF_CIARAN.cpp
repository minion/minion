#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_alldiff_ciaran.h"

template<typename VarArray>
AbstractConstraint*
BuildCT_ALLDIFF_CIARAN(StateObj* stateObj, const VarArray& var_array, ConstraintBlob&)
{ return new AlldiffCiaran<VarArray>(stateObj, var_array); }

BUILD_CT(CT_ALLDIFF_CIARAN, 1)
