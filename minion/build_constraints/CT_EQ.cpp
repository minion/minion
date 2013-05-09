#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_equal.h"

template<typename T1, typename T2>
AbstractConstraint*
BuildCT_EQ(StateObj* stateObj, const T1& t1, const T2& t2, ConstraintBlob&) 
{ return EqualCon(stateObj, t1[0],t2[0]); }

BUILD_CT(CT_EQ, 2)
