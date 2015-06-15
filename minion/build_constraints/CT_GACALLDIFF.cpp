#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_gacalldiff.h"

template<typename VarArray>
AbstractConstraint*
BuildCT_GACALLDIFF(const VarArray& var_array, ConstraintBlob&)
{ return new GacAlldiffConstraint<VarArray>(var_array); }

BUILD_CT(CT_GACALLDIFF, 1)
