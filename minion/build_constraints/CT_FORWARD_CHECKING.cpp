#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/forward_checking.h"


inline AbstractConstraint*
BuildCT_FORWARD_CHECKING(StateObj* stateObj, ConstraintBlob& bl)
{
  D_ASSERT(bl.internal_constraints.size() == 1);
  return forwardCheckingCon(stateObj, build_constraint(stateObj, bl.internal_constraints[0]));
}

BUILD_CT(CT_FORWARD_CHECKING, 0)
