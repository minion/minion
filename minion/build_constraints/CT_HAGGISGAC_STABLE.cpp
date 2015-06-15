#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_haggisgac_stable.h"

template <typename T>
AbstractConstraint*
BuildCT_HAGGISGAC_STABLE(const T& t1, ConstraintBlob& b)
{ return new HaggisGACStable<T>(t1, b.short_tuples); }

BUILD_CT(CT_HAGGISGAC_STABLE, 1)