#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net

   For Licence Information see file LICENSE.txt
*/

#include "../constraints/reify.h"
#include "../constraints/constraint_equal.h"

template<typename VarRef1, typename BoolVarRef>
AbstractConstraint*
BuildCT_DISEQ_REIFY(const vector<VarRef1>& var1,
                                        const vector<VarRef1>& var2, const vector<BoolVarRef> var3, ConstraintBlob&)
{
    return new ReifiedEqualConstraint<VarRef1, VarRef1, BoolVarRef, true>
                                   (var1[0],var2[0], var3[0]);

}

template<typename VarRef1, typename VarRef2, typename BoolVarRef>
AbstractConstraint*
BuildCT_DISEQ_REIFY(const vector<VarRef1>& var1,
                                        const vector<VarRef2>& var2, const vector<BoolVarRef> var3, ConstraintBlob&)
{
    return new ReifiedEqualConstraint<AnyVarRef, AnyVarRef, BoolVarRef, true>
                                   (AnyVarRef(var1[0]),AnyVarRef(var2[0]), var3[0]);

}

BUILD_CT(CT_DISEQ_REIFY, 3)
