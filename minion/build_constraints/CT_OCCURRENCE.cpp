#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_occurrence.h"

template<typename T1, typename T3>
AbstractConstraint*
BuildCT_OCCURRENCE(const T1& t1, const T3& t3, ConstraintBlob& b)
{
  const SysInt val_to_count = checked_cast<SysInt>(b.constants[0][0]);
  return OccEqualCon(t1, val_to_count, t3[0]);
}

BUILD_CT(CT_OCCURRENCE, 2)
