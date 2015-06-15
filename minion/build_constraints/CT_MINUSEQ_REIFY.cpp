#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net

   For Licence Information see file LICENSE.txt
*/

#include "../constraints/reify.h"
#include "../constraints/constraint_equal.h"

template<typename EqualVarRef1, typename BoolVarRef>
AbstractConstraint*
BuildCT_MINUSEQ_REIFY(const vector<EqualVarRef1>& var1,
                                         const vector<EqualVarRef1>& var2, const vector<BoolVarRef> var3,ConstraintBlob&)
{ return new ReifiedEqualConstraint<EqualVarRef1, VarNeg<EqualVarRef1>, BoolVarRef>(var1[0],VarNegRef(var2[0]),var3[0]); }

template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
AbstractConstraint*
BuildCT_MINUSEQ_REIFY(const vector<EqualVarRef1>& var1,
                                         const vector<EqualVarRef2>& var2, const vector<BoolVarRef> var3,ConstraintBlob&)
{ return new ReifiedEqualConstraint<AnyVarRef, AnyVarRef, BoolVarRef>(AnyVarRef(var1[0]),AnyVarRef(VarNegRef(var2[0])),var3[0]); }


BUILD_CT(CT_MINUSEQ_REIFY, 3)
