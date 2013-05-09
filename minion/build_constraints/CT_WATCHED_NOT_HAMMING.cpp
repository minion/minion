#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/dynamic_sumor.h"

template<typename T1, typename T2>
AbstractConstraint*
BuildCT_WATCHED_NOT_HAMMING(StateObj* stateObj, const T1& t1, const T2& t2, ConstraintBlob& b) 
{ return NotVecOrCountConDynamic(stateObj, t1, t2, b.constants[0][0]); }

BUILD_CT(CT_WATCHED_NOT_HAMMING, 2)
