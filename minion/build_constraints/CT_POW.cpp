#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_pow.h"
#include "../constraints/constraint_pow_check.h"
#include "../constraints/forward_checking.h"


template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_POW(StateObj* stateObj, const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  // vars1 is special to avoid 0^0
  if(vars[0].getInitialMin() < 0 ||
     vars[1].getInitialMin() <= 0 ||
     var2[0].getInitialMin() < 0)
  {
    typedef PowConstraint_Check<typename V1::value_type, typename V1::value_type, typename V2::value_type, false> PowConC;
    AbstractConstraint* pow=new CheckAssignConstraint<PowConC, false>(stateObj, PowConC(stateObj, vars[0], vars[1], var2[0]));
    // Now wrap it in new FC thing. Horrible hackery.
    return forwardCheckingCon(stateObj, pow);
  }

  return new PowConstraint<typename V1::value_type, typename V1::value_type,
                           typename V2::value_type>(stateObj, vars[0], vars[1], var2[0]);
}

BUILD_CT(CT_POW, 2)
