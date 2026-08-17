// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

namespace Controller {

inline SysInt getWorldDepth() {
  return getMemory().backTrack().current_depth();
}

/// Pushes the state of the whole world.
inline void worldPush() {
  D_ASSERT(!getState().isFailed());
  getQueue().getTbq().worldPush();
  getMemory().monotonicSet().before_branch_left();
  D_ASSERT(getQueue().isQueuesEmpty());
  getMemory().backTrack().worldPush();
  getMemory().monotonicSet().after_branch_left();
  getState().getConstraintsToPropagate().push_back(set<AbstractConstraint*>());
  getState().getGenericBacktracker().mark();
}

/// Pops the state of the whole world.
inline void worldPop() {
  getState()._unsafeClearFailed();
  getQueue().clearQueues();
  getState().getGenericBacktracker().worldPop();
  getMemory().backTrack().worldPop();
  getMemory().monotonicSet().undo();
  getQueue().getTbq().worldPop();

  // After the memory pop, any var container whose backtrackable numVarsActive
  // has now reverted below its live varCount_m needs its newly-orphaned slots
  // restored to initial bounds. Must happen before re-propagating constraints.
  getVars().boundVarContainer.reinitIfNeeded();
  getVars().boolVarContainer.reinitIfNeeded();
  getVars().bigRangeVarContainer.reinitIfNeeded();
  getVars().sparseBoundVarContainer.reinitIfNeeded();

  vector<set<AbstractConstraint*>>& constraintList = getState().getConstraintsToPropagate();
  SysInt propagateDepth = getWorldDepth() + 1;
  if((SysInt)constraintList.size() > propagateDepth) {
    for(set<AbstractConstraint*>::iterator it = constraintList[propagateDepth].begin();
        it != constraintList[propagateDepth].end(); it++) {
      // Re-establish the constraint's internal structures before
      // propagating with them. A constraint added mid-search built them
      // at the depth it was created at, and anything it keeps in
      // backtrackable memory has just been zeroed by the pop above.
      // init_constraint also reaches the children of a meta-constraint,
      // which never get fullPropagate called on them directly.
      (*it)->init_constraint();
      (*it)->fullPropagate();
    }

    if(propagateDepth > 0) {
      constraintList[propagateDepth - 1].insert(constraintList[propagateDepth].begin(),
                                                constraintList[propagateDepth].end());
    }
    constraintList.pop_back();
  }
}

inline void worldPopToDepth(SysInt depth) {
  // TODO: Speed up this method. It shouldn't call worldPop repeatedly.
  // The main problem is this requires adding additions to things like
  // monotonic sets I suspect.
  D_ASSERT(depth <= getWorldDepth());
  while(depth < getWorldDepth())
    worldPop();
}

inline void worldPop_all() {
  SysInt depth = getMemory().backTrack().current_depth();
  for(; depth > 0; depth--)
    worldPop();
}
} // namespace Controller

inline bool SearchState::addConstraint(AbstractConstraint* c) {
  // Catastrophic if true: setup/fullPropagate would not run, but the
  // constraint pointer would still be silently dropped on the floor —
  // very hard to diagnose later. Callers must check `isFailed()`
  // themselves and bail out cleanly (the public mid-search entry points
  // in libwrapper.cpp do this).
  if(getState().isFailed()) {
    INTERNAL_ERROR("addConstraint called with solver state already failed");
  }
  constraints.push_back(c);
  vector<AnyVarRef>* vars = c->getVarsSingleton();
  size_t vars_s = vars->size();
  for(size_t i = 0; i < vars_s; i++) // note all constraints the var is involved in
    (*vars)[i].addConstraint(c);

  c->setup();
  c->fullPropagate();
  c->fullPropagateDone = true;
  if(getState().isFailed()) {
    return false;
  }
  getQueue().propagateQueueRoot();
  return true;
}

inline bool SearchState::addConstraintMidsearch(AbstractConstraint* c) {
  bool ret = addConstraint(c);
  // Register for re-propagation on every worldPop that undoes the depth
  // at which the constraint was added — whenever the constraint was
  // actually installed (i.e. fullPropagate was called). We must NOT gate
  // this on `ret`: if the initial fullPropagate wiped out a domain,
  // addConstraint returns false, but the constraint has already been
  // pushed into `constraints`, registered with its variables, and placed
  // its dynamic triggers (triggerSetup inside fullPropagate runs before
  // any domain operation that could fail). On backtrack the trail then
  // restores the variables, but unless we re-run fullPropagate, the
  // propagator's own internal invariants (e.g. `abs` enforcing var1>=0
  // with var1.setMin(0)) aren't re-established — and any later trigger
  // firing can then crash the propagator.
  //
  // The early-bailout branch in addConstraint (state already failed on
  // entry) never runs setup/fullPropagate and so leaves fullPropagateDone
  // as its default false — those constraints correctly skip registration.
  //
  // worldPop re-fullPropagates constraints at ctp[propagateDepth] where
  // propagateDepth = newDepth + 1 = oldDepth. So to make the constraint
  // fire on the next pop that undoes the current depth's state, we need
  // it at ctp[currentDepth]. In the healthy invariant ctp.size ==
  // currentDepth, so currentDepth is one past the last valid index —
  // grow the vector by one to make room.
  if(c->fullPropagateDone) {
    SysInt insertIdx = Controller::getWorldDepth();
    if((SysInt)constraintsToPropagate.size() <= insertIdx)
      constraintsToPropagate.resize(insertIdx + 1);
    constraintsToPropagate[insertIdx].insert(c);
  }
  return ret;
}
