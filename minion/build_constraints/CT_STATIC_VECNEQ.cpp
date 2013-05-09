#define  SLOW_VEC_OR

#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/



#include "../dynamic_constraints/dynamic_vecneq.h"

template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
BuildCT_STATIC_VECNEQ(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2, ConstraintBlob&)
{ return new ConName <VarArray1,VarArray2>(stateObj, varray1, varray2); }



BUILD_CT(CT_STATIC_VECNEQ, 2)
