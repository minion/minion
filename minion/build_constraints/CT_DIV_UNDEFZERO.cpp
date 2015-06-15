#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_div.h"
#include "../constraints/constraint_checkassign.h"
#include "../constraints/forward_checking.h"
template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_DIV_UNDEFZERO(const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  typedef DivConstraint<typename V1::value_type, typename V1::value_type, typename V2::value_type, true> DivCon;
  AbstractConstraint* div=new CheckAssignConstraint<DivCon, false>(DivCon(vars[0], vars[1], var2[0]));
  return forwardCheckingCon(div);
}

BUILD_CT(CT_DIV_UNDEFZERO, 2)
