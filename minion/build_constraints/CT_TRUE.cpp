#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_constant.h"

inline AbstractConstraint*
BuildCT_TRUE(StateObj* stateObj, ConstraintBlob&)
{ return (new ConstantConstraint<true>(stateObj)); }

BUILD_CT(CT_TRUE, 0)
