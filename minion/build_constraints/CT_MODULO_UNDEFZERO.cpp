#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_modulo.h"
#include "../constraints/constraint_checkassign.h"
#include "../constraints/forward_checking.h"

template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_MODULO_UNDEFZERO(const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  /*if(vars[0].getInitialMin() < 0 || vars[1].getInitialMin() < 1 ||
         var2[0].getInitialMin() < 0)
  {
    typedef SlowModConstraint<typename V1::value_type, typename V1::value_type, typename V2::value_type, true> ModCon;
    return new CheckAssignConstraint<ModCon, false>(ModCon(vars[0], vars[1], var2[0]));
  }
  else
    return new ModConstraint<typename V1::value_type, typename V1::value_type,
                             typename V2::value_type>(vars[0], vars[1], var2[0]);*/
  // Do FC. Same as CT_MODULO except for last template parameter of SlowModConstraint
  typedef SlowModConstraint<typename V1::value_type, typename V1::value_type, typename V2::value_type, true> ModCon;
  AbstractConstraint* modct=new CheckAssignConstraint<ModCon, false>(ModCon(vars[0], vars[1], var2[0]));
  return forwardCheckingCon(modct);
}

BUILD_CT(CT_MODULO_UNDEFZERO, 2)
