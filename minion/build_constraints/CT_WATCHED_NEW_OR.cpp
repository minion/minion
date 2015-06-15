#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/dynamic_new_or.h"

inline AbstractConstraint*
BuildCT_WATCHED_NEW_OR(ConstraintBlob& bl)
{
  vector<AbstractConstraint*> cons;
  for(SysInt i = 0; i < (SysInt)bl.internal_constraints.size(); ++i)
    cons.push_back(build_constraint(bl.internal_constraints[i]));
  return new Dynamic_OR(cons);
}

BUILD_CT(CT_WATCHED_NEW_OR, 0)
