#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_constant.h"

inline AbstractConstraint*
BuildCT_FALSE(ConstraintBlob&)
{ return (new ConstantConstraint<false>()); }

BUILD_CT(CT_FALSE, 0)
