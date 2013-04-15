#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_shortstr2.h"

template <typename T>
AbstractConstraint*
BuildCT_SHORTSTR(StateObj* stateObj,const T& t1, ConstraintBlob& b)
{ return new STR<T, true>(stateObj, t1, b.short_tuples); }

BUILD_CT(CT_SHORTSTR, 1)