// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef STATEOBJ_HPP
#define STATEOBJ_HPP

#include "StateObj_forward.h"

#include "parallel/parallel.h"

VARDEF(Memory searchMem_m);
VARDEF(SearchOptions options_m);
VARDEF(SearchState state_m);
VARDEF(Queues queues_m);
VARDEF(VariableContainer varContainer_m);
VARDEF(BoolContainer bools_m);

inline BoolContainer& getBools() {
  return bools_m;
}
inline SearchOptions& getOptions() {
  return options_m;
}
inline SearchState& getState() {
  return state_m;
}
inline Queues& getQueue() {
  return queues_m;
}
inline Memory& getMemory() {
  return searchMem_m;
}
inline VariableContainer& getVars() {
  return varContainer_m;
}

namespace Parallel {
VARDEF(ParallelData* parData_m);

inline ParallelData& getParallelData() {
  if(parData_m == 0) {
    parData_m = setupParallelData();
  }
  return *parData_m;
}
} // namespace Parallel

template <typename DomType>
inline BoundVarContainer<DomType>& BoundVarRef_internal<DomType>::getCon_Static() {
  return varContainer_m.boundVarContainer;
}

inline BoolVarContainer& BoolVarRef_internal::getCon_Static() {
  return varContainer_m.boolVarContainer;
}

template <typename DomType>
inline SparseBoundVarContainer<DomType>& SparseBoundVarRef_internal<DomType>::getCon_Static() {
  return varContainer_m.sparseBoundVarContainer;
}

template <typename d_type>
inline BigRangeVarContainer<d_type>& BigRangeVarRef_internal_template<d_type>::getCon_Static() {
  return varContainer_m.bigRangeVarContainer;
}

// Must be defined later.
inline SearchState::~SearchState() {
  for(SysInt i = 0; i < (SysInt)constraints.size(); ++i)
    delete constraints[i];
}

template <typename Var>
std::string getBaseVarName(const Var& v) {
  return getState().getInstance()->vars.getName(v.getBaseVar());
}

#endif
