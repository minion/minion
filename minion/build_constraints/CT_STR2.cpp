#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_shortstr2.h"

template <typename T>
AbstractConstraint*
BuildCT_STR(const T& t1, ConstraintBlob& b)
{ return new STR<T, false>(t1, b.tuples); }

BUILD_CT(CT_STR, 1)