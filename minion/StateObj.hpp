// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef STATEOBJ_HPP
#define STATEOBJ_HPP

#include "StateObj_forward.h"

#include "globals.h"
#include "parallel/parallel.h"

VARDEF(Memory searchMem_m);
VARDEF(SearchOptions options_m);
VARDEF(SearchState state_m);
VARDEF(Queues queues_m);
VARDEF(VariableContainer varContainer_m);
VARDEF(BoolContainer bools_m);

/*
 * Libminion case:
 *
 * Pointer trickery as compiler doesnt like globals.x when there are still
 * incomplete types (such as SearchOptions, ...).
 *
 * Tried rearranging headerfiles, didn't work, so am lazily creating them when referenced.
 * instead.
 */

inline BoolContainer& getBools() {
#ifdef LIBMINION
  if(GET_GLOBAL(bools_m) == 0) {
    GET_GLOBAL(bools_m) = new BoolContainer();
  }
  return *GET_GLOBAL(bools_m);
#else
  return GET_GLOBAL(bools_m);
#endif
}
inline SearchOptions& getOptions() {
#ifdef LIBMINION
  if(GET_GLOBAL(options_m) == 0) {
    GET_GLOBAL(options_m) = new SearchOptions();
  }
  return *GET_GLOBAL(options_m);
#else
  return GET_GLOBAL(options_m);
#endif
}
inline SearchState& getState() {
#ifdef LIBMINION
  if(GET_GLOBAL(state_m) == 0) {
    GET_GLOBAL(state_m) = new SearchState();
  }
  return *GET_GLOBAL(state_m);
#else
  return GET_GLOBAL(state_m);
#endif
}
inline Queues& getQueue() {
#ifdef LIBMINION
  if(GET_GLOBAL(queues_m) == 0) {
    GET_GLOBAL(queues_m) = new Queues();
  }
  return *GET_GLOBAL(queues_m);
#else
  return GET_GLOBAL(queues_m);
#endif
}
inline Memory& getMemory() {
#ifdef LIBMINION
  if(GET_GLOBAL(searchMem_m) == 0) {
    GET_GLOBAL(searchMem_m) = new Memory();
  }
  return *GET_GLOBAL(searchMem_m);
#else
  return GET_GLOBAL(searchMem_m);
#endif
}
inline VariableContainer& getVars() {
#ifdef LIBMINION
  if(GET_GLOBAL(varContainer_m) == 0) {
    GET_GLOBAL(varContainer_m) = new VariableContainer();
  }
  return *GET_GLOBAL(varContainer_m);
#else
  return GET_GLOBAL(varContainer_m);
#endif
}

VARDEF(Parallel::ParallelData* parData_m);
inline Parallel::ParallelData& getParallelData() {

  if(GET_GLOBAL(parData_m) == 0) {
    GET_GLOBAL(parData_m) = Parallel::setupParallelData();
  }
  return *GET_GLOBAL(parData_m);
}

template <typename DomType>
inline BoundVarContainer<DomType>& BoundVarRef_internal<DomType>::getCon_Static() {
  return getVars().boundVarContainer;
}

inline BoolVarContainer& BoolVarRef_internal::getCon_Static() {
  return getVars().boolVarContainer;
}

template <typename DomType>
inline SparseBoundVarContainer<DomType>& SparseBoundVarRef_internal<DomType>::getCon_Static() {
  return getVars().sparseBoundVarContainer;
}

template <typename d_type>
inline BigRangeVarContainer<d_type>& BigRangeVarRef_internal_template<d_type>::getCon_Static() {
  return getVars().bigRangeVarContainer;
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
