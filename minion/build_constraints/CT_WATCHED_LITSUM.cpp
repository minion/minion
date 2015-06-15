#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/dynamic_literalwatch.h"

template<typename T1>
AbstractConstraint* 
BuildCT_WATCHED_LITSUM(const T1& t1, ConstraintBlob& b)
{
  D_ASSERT(b.constants[1].size());
  return LiteralSumConDynamic(t1, b.constants[0], checked_cast<SysInt>(b.constants[1][0])); 
}

BUILD_CT(CT_WATCHED_LITSUM, 1)
