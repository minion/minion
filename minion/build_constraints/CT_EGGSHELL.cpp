#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_eggshell.h"

template <typename T>
AbstractConstraint*
BuildCT_EGGSHELL(StateObj* stateObj,const T& t1, ConstraintBlob& b)
{ return new EggShell<T>(stateObj, t1, b.tuples); }

BUILD_CT(CT_EGGSHELL, 1)