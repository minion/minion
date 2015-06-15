#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_less.h"

template<typename T1, typename T2>
AbstractConstraint*
BuildCT_INEQ(const T1& t1, const T2& t2, ConstraintBlob& b) 
{ return LeqCon(t1[0], t2[0], b.constants[0][0]); }

BUILD_CT(CT_INEQ, 2)
