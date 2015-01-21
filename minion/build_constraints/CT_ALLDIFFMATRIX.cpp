#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_alldiffmatrix.h"

template<typename VarArray>
AbstractConstraint*
BuildCT_ALLDIFFMATRIX(StateObj* stateObj, const VarArray& var_array, ConstraintBlob& b)
{ return new AlldiffMatrixConstraint<VarArray, decltype(b.constants[0][0])>(stateObj, var_array, b.constants[0][0]); }

BUILD_CT(CT_ALLDIFFMATRIX, 1)
