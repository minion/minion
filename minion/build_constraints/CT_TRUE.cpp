#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_constant.h"

inline AbstractConstraint*
BuildCT_TRUE(ConstraintBlob&)
{ return (new ConstantConstraint<true>()); }

BUILD_CT(CT_TRUE, 0)
