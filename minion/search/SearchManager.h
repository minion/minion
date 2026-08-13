// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include "../preprocess.h"
#include "common_search.h"
#include "variable_orderings.h"
#include "../BuildVariables.h"
#include "../parallel/work_steal.h"

namespace Controller {

template <typename T>
void inline maybe_print_search_assignment(T& var, DomainInt val, BOOL equal) {
  if(getOptions().dumptree) {
    cout << "SearchAssign:" << var << (equal ? " = " : " != ") << val << endl;
  }
  if(getOptions().dumptreeobj) {
    getOptions().dumptreeobj->branch(getState().getNodeCount(), getBaseVarName(var), val, equal);
  }
}

// instead of carrying around the pos everywhere, the VariableOrder object has
// pos in it as a reversible<SysInt>.

struct SearchManager {
  inline virtual ~SearchManager() {}

  virtual void search() = 0;

  // Append a freshly-created variable into the live search's aux block.
  // Implementations route to the aux StaticBranch and keep any cached
  // var-list in sync. Default aborts — it's a bug to call this on a
  // manager type that doesn't support mid-search additions.
  virtual void appendAuxVar(AnyVarRef v, ValOrder vo) {
    (void)v;
    (void)vo;
    INTERNAL_ERROR("appendAuxVar not implemented on this SearchManager");
  }

  // Replay a path-from-root into this worker's state, fast-forwarding to
  // a stolen sub-tree under -X-parallelWorkSteal. Default refuses; only
  // StandardSearchManager (the work-stealing-supported manager) overrides.
  virtual bool replayPath(const WorkSteal::WorkItem&) {
    INTERNAL_ERROR("replayPath not supported on this SearchManager type");
    return false;
  }

  // Clear the branch stack — used by the work-stealing worker loop
  // between sub-trees so the next replayPath sees an empty stack.
  // Default no-op (no branches to clear).
  virtual void reset() {}
};

struct StandardSearchManager : public SearchManager {

  inline virtual ~StandardSearchManager() {}

  std::function<void(const vector<AnyVarRef>&, const vector<Controller::triple>&)> check_func;
  std::function<void(void)> handle_sol_func, handle_opt_func;

  vector<AnyVarRef> varArray;
  shared_ptr<VariableOrder> varOrder;

  shared_ptr<Propagate> prop; // Propagate is the type of the base class. Method
                              // prop->prop(varArray)

  vector<Controller::triple> branches; // L & R branches so far (isLeftBranch?,var,value)

  StandardSearchManager(
      shared_ptr<VariableOrder> _varOrder, shared_ptr<Propagate> _prop,
      std::function<void(const vector<AnyVarRef>&, const vector<Controller::triple>&)> _check_func,
      std::function<void(void)> _handle_sol_func, std::function<void(void)> _handle_opt_func)
      : check_func(_check_func),
        handle_sol_func(_handle_sol_func),
        handle_opt_func(_handle_opt_func),
        varOrder(_varOrder),
        prop(_prop) {
    varArray = varOrder->getVars();
    branches.reserve(varArray.size());
  }

  void reset() {
    branches.clear();
  }

  virtual void appendAuxVar(AnyVarRef v, ValOrder vo) {
    MultiBranch* mb = dynamic_cast<MultiBranch*>(varOrder.get());
    CHECK(mb != nullptr, "appendAuxVar: live VariableOrder is not a MultiBranch");
    StaticBranch* aux = mb->getAuxStaticBranch();
    CHECK(aux != nullptr, "appendAuxVar: no aux StaticBranch available");
    aux->varOrder.push_back(v);
    aux->valOrder.push_back(vo);
    // Keep the propagator-pass/var-check list consistent with the live
    // search order. Indices into varArray are no longer load-bearing
    // (pickVarVal returns AnyVarRef directly), so appending is safe.
    varArray.push_back(v);
  }

  // returns false if left branch not possible.
  inline void branch_left(const BranchChoice& picked) {
    D_ASSERT(picked.var.data);
    D_ASSERT(!picked.var.isAssigned());

    worldPush();
    AnyVarRef var = picked.var;
    var.assign(picked.val);
    maybe_print_search_assignment(var, picked.val, true);
    branches.push_back(Controller::triple(true, var, picked.val, picked.isAux));
  }

  inline bool branch_right() {
    while(!branches.empty() && !branches.back().isLeft) { // pop off all the
                                                          // RBs
      maybe_print_right_backtrack();
      D_ASSERT(!branches.back().stolen);
      branches.pop_back();
    }

    if(branches.empty())
      return false;

    worldPop();

    AnyVarRef var = branches.back().var;
    DomainInt val = branches.back().val;
    bool isAux = branches.back().isAux;
    bool isLeft = branches.back().isLeft;
    (void)isLeft;

    bool stolen = branches.back().stolen;

    D_ASSERT(isLeft);
    // remove the left branch.
    branches.pop_back();

    // We can no longer assert var.inDomain(val): a constraint added
    // mid-search and re-propagated by this worldPop may already have
    // pruned `val`. There is then nothing left to remove, so skip the
    // domain operation and just record the right branch marker; search
    // progresses correctly. Falling through to removeFromDomain is NOT
    // harmless: on a bound variable it is a hard USER_ERROR (the
    // container cannot represent a hole), which exit(1)s the process.

    // special case the upper and lower bounds to make it work for bound
    // variables
    if(!var.inDomain(val)) {
      // nothing to do
    } else if(var.min() == val) {
      var.setMin(val + 1);
    } else if(var.max() == val) {
      var.setMax(val - 1);
    } else {
      var.removeFromDomain(val);
    }
    maybe_print_search_assignment(var, val, false);
    branches.push_back(Controller::triple(false, var, val, isAux));

    // If this branch was stolen, then we want to carry on
    // backtracking
    if(stolen)
      return branch_right();
    else
      return true;
  }

  inline bool in_aux_vars() {
    return !branches.empty() && branches.back().isAux;
  }

  inline void jump_out_aux_vars() {
    while(in_aux_vars()) {
      if(branches.back().isLeft) {
        worldPop();
        maybe_print_right_backtrack();
      }

      branches.pop_back();
    }
  }

  // Fast-forward this worker into the sub-tree described by `path` (a
  // sequence of branch decisions from the root). For each step:
  //   - effectiveLeft = true: worldPush + var.assign(value) + propagate
  //   - effectiveLeft = false: worldPush + remove(value) + propagate
  // Pre-steal left-branch entries are marked stolen=true so that when
  // search exhausts the donated sub-tree and backtracks past the steal
  // point, branch_right's recursive stolen-handling cleanly drains the
  // stack and returns false (search loop exits without re-exploring
  // ancestors that belong to other workers).
  //
  // Returns true if the replayed state is feasible (search should
  // continue), false if propagation derived UNSAT during replay (the
  // donated sub-tree was vacuous; the worker should just go idle again).
  // On false return, the world stack and branches are restored to the
  // state at entry, so the worker can be reused for the next work item.
  bool replayPath(const WorkSteal::WorkItem& item) {
    D_ASSERT(branches.empty());
    if(item.path.empty())
      return false;
    SysInt steal_idx = (SysInt)item.path.size() - 1;
    // Track how many worldPush calls we issued, so we can unwind exactly
    // them on a mid-replay propagation failure. This is NOT
    // path.size(): right-branch entries do not push (they mirror
    // branch_right which only worldPops and modifies; only branch_left
    // pushes).
    SysInt pushed = 0;
    for(SysInt i = 0; i < (SysInt)item.path.size(); ++i) {
      const WorkSteal::BranchStep& s = item.path[i];
      ::ProbSpec::Var v((VariableType)s.varType, (DomainInt)s.varPos);
      AnyVarRef var = BuildCon::getAnyVarRefFromVar(v);
      DomainInt val(checked_cast<SysInt>(s.value));
      if(s.effectiveLeft) {
        // Mirror branch_left: worldPush + assign + propagate.
        worldPush();
        ++pushed;
        var.assign(val);
      } else {
        // Mirror branch_right's L→R conversion: NO worldPush. The trail
        // level stays where it is (which matches the donor's state at
        // this point, because the donor's R came from a worldPop that
        // undid the L push). Apply the right-side modification directly.
        if(var.min() == val)
          var.setMin(val + 1);
        else if(var.max() == val)
          var.setMax(val - 1);
        else
          var.removeFromDomain(val);
      }
      prop->prop(varArray);
      // Pre-steal left entries are marked stolen so the eventual
      // backtrack drains them recursively (each recursion calls
      // worldPop, balancing the worldPush we did above). The final
      // step is the steal point itself — never stolen, since that's
      // the right-branch the stealer is here to explore. Right entries
      // are never marked stolen (the assert in branch_right forbids it
      // and they don't have a worldPush to balance anyway).
      bool stolen = (i < steal_idx) && s.effectiveLeft;
      Controller::triple t(s.effectiveLeft, var, val, false);
      t.stolen = stolen;
      branches.push_back(t);
      if(getState().isFailed()) {
        // Replay infeasible: undo exactly the worldPushes we issued
        // (one per L step taken so far, NOT one per path step) and
        // clear branches so the worker is reusable for the next item.
        for(SysInt j = 0; j < pushed; ++j) {
          worldPop();
        }
        branches.clear();
        return false;
      }
    }
    return true;
  }

  // Steal work isn't used, but is left here in case anyone ever wants it.
  int steal_work() { // find where to steal the topmost left branch from this search.
    for(UnsignedSysInt newceil = 0; newceil < branches.size() - 1; ++newceil) {
      if(branches[newceil].isLeft && !branches[newceil].stolen) {
        return newceil;
      }
    }
    return -1;
  }

  // Most basic search procedure
  virtual void search() {
    maybe_print_node();
    while(true) {
      D_ASSERT(getQueue().isQueuesEmpty());

      getState().incrementNodeCount();

      check_func(varArray, branches);

      BranchChoice varval = varOrder->pickVarVal();

      if(!varval.var.data) {
        // We have found a solution!
        check_sol_is_correct();
        maybe_print_node(true);
        handle_sol_func();
        if(varOrder->hasAuxVars()) { // There are AUX vars at the end of the var ordering.
          // Backtrack out of them.
          jump_out_aux_vars();
        }

        // If we are not finished, then go into the loop below.
        getState().setFailed();
      } else {
        maybe_print_node();
        branch_left(varval);
        prop->prop(varArray);
      }

      // Work-stealing donation poll. When at least one worker is idle, the
      // current (busy) worker hands off its topmost stealable left branch
      // by encoding the path-from-root and pushing to the shared queue.
      // Reuses the existing `branches[steal].stolen = true` mechanism so
      // backtrack past a stolen branch correctly skips its right side
      // (SearchManager.h:145-146 inside branch_right).
      //
      // Gate on workStealController != nullptr (set by the orchestrator
      // for every worker context) rather than numWorkStealThreads — the
      // latter is zeroed in worker copies to prevent recursion.
      if(getOptions().workStealController != nullptr && !getState().isFailed() &&
         !in_aux_vars()) {
        WorkSteal::WorkStealController* ctrl =
            static_cast<WorkSteal::WorkStealController*>(getOptions().workStealController);
        if(ctrl->idleWorkers.load(std::memory_order_acquire) > 0) {
          int steal = steal_work();
          if(steal != -1) {
            WorkSteal::WorkItem item;
            item.path.reserve(steal + 1);
            for(int i = 0; i <= steal; ++i) {
              WorkSteal::BranchStep step;
              ::ProbSpec::Var v = branches[i].var.getBaseVar();
              step.varType = (uint8_t)v.type();
              step.varPos = (uint32_t)v.pos();
              step.value = checked_cast<long long>(branches[i].val);
              // Preserve the donor's direction for indices < steal; flip
              // the steal point itself (donor is on left, stealer takes
              // right).
              step.effectiveLeft = (i < steal) ? branches[i].isLeft : false;
              item.path.push_back(step);
            }
            branches[steal].stolen = true;
            WorkSteal::donate(*ctrl, std::move(item));
          }
        }
      }

      if(getOptions().parallel && !getState().isFailed() && !in_aux_vars()) {
        if(getOptions().parallelStealHigh) {
          bool doFork = Parallel::shouldDoFork();

          int steal = steal_work();

          int isParent = 0;

          // std::cerr << "Fork?" << getState().isFailed() << "\n";;
          if(doFork) {
            // std::cerr << "Yes, do a fork!\n";

            isParent = Parallel::doFork();

            if(isParent) {
              if(steal != -1) {
                branches[steal].stolen = true;
              }
            } else {
              // Todo, avoid this process creation?
              if(steal == -1) {
                Parallel::endParallelMinion();
                exit(0);
              }
              // lockSolsout();
              // std::cerr << "stealing " << steal << "\n";
              // std::cerr << branches << "\n";
              while(branches.size() > steal + 1) {
                // std::cerr << "right branch: " << branches << "\n";
                int ret = branch_right();
                if(!ret) {
                  D_FATAL_ERROR("Error in parallel forking");
                }
              }
              D_ASSERT(branches.size() == steal + 1);
              prop->prop(varArray);
              // std::cerr << branches << "\n";
              // unlockSolsout();
            }
          }
        } else {
          bool doFork = Parallel::shouldDoFork();
          if(doFork) {
            // std::cerr << "Yes, do a fork!\n";

            int isParent = Parallel::doFork();
            D_CHECK(isParent >= 0);
            if(isParent) {
              // Force to ignore left branch
              getState().setFailed();
            } else {
              // Force to stay in left branch
              reset();
            }
          }
        }
      }

      // loop to
      while(getState().isFailed()) {
        if(branches.size() == 0) {
          return;
        }

        maybe_print_backtrack();
        bool flag = branch_right();
        if(!flag) { // No remaining left branches to branch right.
          return;
        }
        // Deal with optimisation variables
        getState().incrementBacktrackCount();
        handle_opt_func();
        if(!getState().isFailed()) {
          prop->prop(varArray);
        }
      }
    }
  }
};
} // namespace Controller

#endif
