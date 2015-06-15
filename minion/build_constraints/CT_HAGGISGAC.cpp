#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_haggisgac_bt.h"

template <typename T>
AbstractConstraint*
BuildCT_HAGGISGAC(const T& t1, ConstraintBlob& b)
{ return new HaggisGAC<T>(t1, b.short_tuples); }

BUILD_CT(CT_HAGGISGAC, 1)
