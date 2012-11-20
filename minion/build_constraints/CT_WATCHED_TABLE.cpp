#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_GACtable_master.h"

template <typename T>
AbstractConstraint*
BuildCT_WATCHED_TABLE(StateObj* stateObj,const T& t1, ConstraintBlob& b)
{ return GACTableCon(stateObj, t1, b.tuples); }

BUILD_CT(CT_WATCHED_TABLE, 1)
