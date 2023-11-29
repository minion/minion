// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "BuildVariables.h"


namespace BuildCon {

SysInt same_type_check(ConstraintBlob& b, SysInt pos) {
  const vector<Var>& vars = b.vars[pos];

  // type needs to be something for empty arrays
  SysInt type = VAR_CONSTANT;
  bool same_type = true;

  if(!vars.empty()) {
    type = vars[0].type();
    for(UnsignedSysInt i = 1; i < vars.size(); ++i) {
      if(vars[i].type() != type) {
        same_type = false;
        break;
      }
    }
  }

  if(same_type) {
    return type;
  }
  else {
    return -123;
  }
}

vector<BoolVarRef> make_boolvarref(const vector<Var>& vars) {
      vector<BoolVarRef> v(vars.size());
      for(UnsignedSysInt i = 0; i < vars.size(); ++i)
        v[i] = getVars().boolVarContainer.getVarNum(vars[i].pos());
      return v;
}

 vector<VarNot<BoolVarRef>> make_neg_boolref(const vector<Var>& vars) {

 vector<VarNot<BoolVarRef>> v(vars.size());
      for(UnsignedSysInt i = 0; i < vars.size(); ++i)
        v[i] = VarNotRef(getVars().boolVarContainer.getVarNum(vars[i].pos()));
  return v;
 }

       vector<BoundVarRef> make_boundref(const vector<Var>& vars) {

       vector<BoundVarRef> v(vars.size());
      for(UnsignedSysInt i = 0; i < vars.size(); ++i)
        v[i] = getVars().boundVarContainer.getVarNum(vars[i].pos());
  return v;
       }

vector<SparseBoundVarRef> make_sparseboundref(const vector<Var>& vars) {
             vector<SparseBoundVarRef> v(vars.size());
      for(UnsignedSysInt i = 0; i < vars.size(); ++i)
        v[i] = getVars().sparseBoundVarContainer.getVarNum(vars[i].pos());
  return v;
}

vector<BigRangeVarRef> make_bigrangeref(const vector<Var>& vars) {

      vector<BigRangeVarRef> v(vars.size());
      for(UnsignedSysInt i = 0; i < vars.size(); ++i)
        v[i] = getVars().bigRangeVarContainer.getVarNum(vars[i].pos());
return v;
}

vector<ConstantVar> make_constref(const vector<Var>& vars) {
      vector<ConstantVar> v(vars.size());
      for(UnsignedSysInt i = 0; i < vars.size(); ++i)
        v[i] = ConstantVar(vars[i].pos());
  return v;
}

vector<AnyVarRef> make_avrref(const vector<Var>& vars) {
 vector<AnyVarRef> v(vars.size());
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
      v[i] = getAnyVarRefFromVar(vars[i]);
return v;
}

}