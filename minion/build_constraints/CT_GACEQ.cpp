#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_gacequal.h"

template<typename T1, typename T2>
AbstractConstraint*
BuildCT_GACEQ(StateObj* stateObj, const T1& t1, const T2& t2, ConstraintBlob&) 
{ return new GACEqualConstraint<typename T1::value_type, typename T2::value_type>(stateObj, t1[0],t2[0]); }

BUILD_CT(CT_GACEQ, 2)

