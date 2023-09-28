// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

namespace BuildCon {

vector<AnyVarRef> getAnyVarRefFromVar(const vector<Var>& vec) {
  vector<AnyVarRef> retVec;
  retVec.reserve(vec.size());

  for(SysInt i = 0; i < (SysInt)vec.size(); ++i)
    retVec.push_back(getAnyVarRefFromVar(vec[i]));

  return retVec;
}

vector<vector<AnyVarRef>> getAnyVarRefFromVar(const vector<vector<Var>>& vec) {
  vector<vector<AnyVarRef>> retVec;
  retVec.reserve(vec.size());

  for(SysInt i = 0; i < (SysInt)vec.size(); ++i)
    retVec.push_back(getAnyVarRefFromVar(vec[i]));

  return retVec;
}

template <typename V>
std::pair<DomainInt, DomainInt> getInitialBounds(const V& v) {
  return std::make_pair(v.initialMin(), v.initialMax());
}

std::pair<DomainInt, DomainInt> getInitialBoundsFromVar(Var v) {
  switch(v.type()) {
  case VAR_BOOL: return getInitialBounds(getVars().boolVarContainer.getVarNum(v.pos()));
  case VAR_NOTBOOL:
    return getInitialBounds(VarNotRef(getVars().boolVarContainer.getVarNum(v.pos())));
  case VAR_BOUND: return getInitialBounds(getVars().boundVarContainer.getVarNum(v.pos()));
  case VAR_SPARSEBOUND:
    return getInitialBounds(getVars().sparseBoundVarContainer.getVarNum(v.pos()));
  case VAR_DISCRETE: return getInitialBounds(getVars().bigRangeVarContainer.getVarNum(v.pos()));
  case VAR_SPARSEDISCRETE: INPUT_ERROR("Sparse Discrete not supported at present");
  case VAR_CONSTANT: return getInitialBounds(ConstantVar(v.pos()));
  default: INPUT_ERROR("Unknown variable type " << v.type() << ".");
  }
}

/// Helper function used in a few places.
AnyVarRef getAnyVarRefFromVar(Var v) {
  switch(v.type()) {
  case VAR_BOOL: return AnyVarRef(getVars().boolVarContainer.getVarNum(v.pos()));
  case VAR_NOTBOOL: return AnyVarRef(VarNotRef(getVars().boolVarContainer.getVarNum(v.pos())));
  case VAR_BOUND: return AnyVarRef(getVars().boundVarContainer.getVarNum(v.pos()));
  case VAR_SPARSEBOUND: return AnyVarRef(getVars().sparseBoundVarContainer.getVarNum(v.pos()));
  case VAR_DISCRETE: return AnyVarRef(getVars().bigRangeVarContainer.getVarNum(v.pos()));
  case VAR_SPARSEDISCRETE: INPUT_ERROR("Sparse Discrete not supported at present");
  case VAR_CONSTANT: return AnyVarRef(ConstantVar(v.pos()));
  default: INPUT_ERROR("Unknown variable type " << v.type() << ".");
  }
}

AnyVarRef getAnyVarRefFromString(const CSPInstance& c, const std::string& s) {
  Var v = c.vars.getSymbol(s);
  return getAnyVarRefFromVar(v);
}


/// Create all the variables used in the CSP.
void buildVariables(const ProbSpec::VarContainer& vars) {
  for(int i = 0; i < vars.BOOLs; ++i)
    getVars().boolVarContainer.addVariables(1);
  for(int i = 0; i < vars.bound.size(); ++i)
    getVars().boundVarContainer.addVariables(vars.bound[i], 1);

  for(int i = 0; i < vars.sparseBound.size(); ++i)
    getVars().sparseBoundVarContainer.addVariables(vars.sparseBound[i], 1);

  getVars().bigRangeVarContainer.addVariables(vars.discrete);

  for(SysInt i = 0; i < (SysInt)vars.sparseDiscrete.size(); ++i) {
    INPUT_ERROR("Sparse discrete disabled at present due to bugs. Sorry.");
  }
}
} // namespace BuildCon
