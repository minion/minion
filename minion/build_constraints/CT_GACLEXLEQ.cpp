#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_gaclex.h"

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
BuildCT_GACLEXLEQ(StateObj* stateObj, const VarArray1& x, const VarArray2& y, ConstraintBlob&)
{ return new GacLexLeqConstraint<VarArray1, VarArray2>(stateObj,x,y); }

BUILD_CT(CT_GACLEXLEQ, 2)
