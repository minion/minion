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

  vector<set<AbstractConstraint*>>& constraintList = getState().getConstraintsToPropagate();
  SysInt propagateDepth = getWorldDepth() + 1;
  if((SysInt)constraintList.size() > propagateDepth) {
    for(set<AbstractConstraint*>::iterator it = constraintList[propagateDepth].begin();
        it != constraintList[propagateDepth].end(); it++) {
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
  if(getState().isFailed()) {
    return false;
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

inline void SearchState::addConstraintMidsearch(AbstractConstraint* c) {
  addConstraint(c);
  c->setup();
  redoFullPropagate(c);
}

inline void SearchState::redoFullPropagate(AbstractConstraint* c) {
  constraintsToPropagate[Controller::getWorldDepth()].insert(c);
  c->fullPropagate();
}
