#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../dynamic_constraints/dynamic_vecneq.h"

template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
BuildCT_WATCHED_VECNEQ(const VarArray1& varray1, const VarArray2& varray2, ConstraintBlob&)
{ return new ConName <VarArray1,VarArray2>(varray1, varray2); }

// these two don't seem to be used anywhere
template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
BuildCT_WATCHED_VEC_OR_LESS(const VarArray1& varray1, const VarArray2& varray2, ConstraintBlob&)
{ return new ConName <VarArray1,VarArray2, LessIterated>(varray1, varray2); }

template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
BuildCT_WATCHED_VEC_OR_AND(const VarArray1& varray1, const VarArray2& varray2, ConstraintBlob&)
{ return new ConName <VarArray1,VarArray2, BothNonZeroIterated>(varray1, varray2); }

BUILD_CT(CT_WATCHED_VECNEQ, 2)
