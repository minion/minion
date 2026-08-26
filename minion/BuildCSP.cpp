// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "parallel/parallel.h"
#include "parallel/work_steal.h"
#include "preprocess.h"
#include "search/search_control.h"

#include "search/restartNewSearchManager.h"

#include "dump_state.hpp"

using namespace ProbSpec;

void SetupCSPOrdering(CSPInstance& instance, SearchMethod args) {
  for(SysInt i = (SysInt)instance.searchOrder.size() - 1; i >= 0; --i) {
    if(args.order != ORDER_NONE) {
      //  For each varorder block, overwrite with order given on the command
      //  line.
      instance.searchOrder[i].order = args.order;
      instance.searchOrder[i].limit = args.limit;
    }

    if(args.valorder != ValOrder(VALORDER_NONE)) {
      //  For each varorder block, overwrite with order given on the command
      //  line.
      for(UnsignedSysInt j = 0; j < instance.searchOrder[i].valOrder.size(); ++j) {
        instance.searchOrder[i].valOrder[j] = args.valorder;
      }
    }

    for(SysInt j = 0; j < (SysInt)instance.searchOrder[i].varOrder.size();
        j++) { // cobble together all the varorder blocks for preprocessing.
      instance.preprocess_vars.push_back(instance.searchOrder[i].varOrder[j]);
    }

    if(getOptions().randomiseValvarorder) {

      std::shuffle(instance.searchOrder[i].varOrder.begin(),
                          instance.searchOrder[i].varOrder.end(), GET_GLOBAL(global_random_gen));

      for(UnsignedSysInt j = 0; j < instance.searchOrder[i].valOrder.size(); ++j) {
        instance.searchOrder[i].valOrder[j] = VALORDER_RANDOM;
      }
    }

    D_ASSERT(instance.searchOrder[i].varOrder.size() == instance.searchOrder[i].valOrder.size());
  }
}
void BuildCSP(CSPInstance& instance) {
  getState().setTupleListContainer(instance.tupleListContainer);
  getState().setShortTupleListContainer(instance.shortTupleListContainer);

  // Set up variables
  BuildCon::buildVariables(instance.vars);
  getState().setInstance(&instance);

  // Set up optimisation
  if(instance.is_optimisation_problem) {
    if(instance.optimiseMinimising)
      Controller::optimiseMinimiseVars(BuildCon::getAnyVarRefFromVar(instance.optimiseVariables));
    else
      Controller::optimiseMaximiseVars(BuildCon::getAnyVarRefFromVar(instance.optimiseVariables));
  }

  vector<vector<AnyVarRef>>& print_matrix = getState().getPrintMatrix();

  // Reserve room in vector - no necessary but more efficent.
  print_matrix.reserve(instance.print_matrix.size());
  for(UnsignedSysInt i = 0; i < instance.print_matrix.size(); ++i)
    print_matrix.push_back(BuildCon::getAnyVarRefFromVar(instance.print_matrix[i]));

  if(getOptions().dumptreeobj) {
    getOptions().dumptreeobj->initialVariables(getVars().getAllVars());
  }
  // Impose Constraints
  for(list<ConstraintBlob>::iterator it = instance.constraints.begin();
      it != instance.constraints.end(); it++) {
        if(!getState().isFailed()) {
          getState().addConstraint(build_constraint(*it));
        }
  }

  // Solve!
  getState().getOldTimer().maybePrintTimestepStore(getOutput(), "Setup Time: ", "SetupTime", getTableOut(),
                                                   !getOptions().silent);
  Controller::initalise_search();
  getState().getOldTimer().maybePrintTimestepStore(getOutput(), "Initial Propagate: ", "InitialPropagate",
                                                   getTableOut(), !getOptions().silent);
}

bool PreprocessCSP(CSPInstance& instance, SearchMethod args) {
    if(getState().isFailed()) {
      return false;
    }

    vector<AnyVarRef> preprocess_anyvars = getAnyVarRefFromVar(instance.preprocess_vars);

    try {
      PropogateCSP(std::max(args.preprocess, args.propMethod), preprocess_anyvars,
                   !getOptions().silent);
    } catch(EndOfSearch eos) {
      return false;
    }

    // This is for savile-row preprocessing
    if(getOptions().gatherAMOs || getOptions().gatherAMOsExtra) {
      collectAMOs(preprocess_anyvars);
    }


    return true;
}
void SolveCSP(CSPInstance& instance, SearchMethod args) {
  // Check that when searching PropagateSAC does actually do the SAC over all
  // vars in any
  // varorder block, not just the ones in the 'current' block

  // Set up variable and value ordering
  // Strange that when using randomiseValvarorder, the variables are
  // only shuffled within the VARORDER blocks from the input file.
  // Likewise, using a dynamic variable ordering, it only applies within
  // the VARORDER blocks.

  shared_ptr<Controller::SearchManager> sm;

  if(getOptions().restart.active) {
    // Optimisation forces sollimit=-1 (findAllSolutions) so that
    // dealWith_solution doesn't terminate after the first solution
    // and search can continue tightening the bound. SAT mode under
    // restarts uses sollimit=1 (find one solution then stop). Any
    // other sollimit is meaningless for restarts — reject.
    bool isOpt = getState().isOptimisationProblem();
    if(!isOpt && getOptions().sollimit != 1) {
      // A usage mistake, not a bug: say what to do rather than asking for a
      // bug report.
      std::cerr << "-restarts abandons the search and begins again, so it can only look for a\n"
                << "single solution.  On a problem with no optimisation objective it cannot be\n"
                << "combined with -findallsols or -sollimit.\n"
                << "Drop -restarts, or ask for one solution.\n";
      exit(1);
    }
    if(isOpt && getOptions().sollimit != -1) {
      std::cerr << "-restarts on an optimisation problem searches until it can prove the best\n"
                << "value, so it cannot be combined with -sollimit.\n"
                << "Drop one of the two.\n";
      exit(1);
    }
    sm = Controller::make_restart_new_search_manager(args.propMethod, instance.searchOrder);
  } else {
    sm = Controller::makeSearch_manager(args.propMethod, instance.searchOrder);
  }

  getState().setSearchManager(sm.get());
  if(getOptions().workStealController != nullptr && getOptions().workStealWorkerIdx >= 0) {
    // Work-stealing worker. Worker 0 starts at the root; workers >= 1
    // skip the initial search and go straight to the queue. After each
    // sub-tree completes, all workers loop on the queue until the
    // controller decides we're done.
    WorkSteal::WorkStealController* ctrl =
        static_cast<WorkSteal::WorkStealController*>(getOptions().workStealController);
    int idx = getOptions().workStealWorkerIdx;
    if(idx == 0) {
      // Controller pre-set busyWorkers = 1 before spawning, so we
      // don't re-bootstrap here. Bookend the root search with
      // worldPush/worldPop: the final branch_right's right-side
      // modification lands at the entry trail level and is otherwise
      // never undone. Without this, those modifications corrupt
      // subsequent replays and lose solutions non-deterministically.
      //
      // Skip search entirely if preprocess (or initial propagation)
      // already determined UNSAT — worldPush asserts isFailed must be
      // false. Mark idle either way so popOrFinish's all-done
      // predicate balances the controller's pre-bootstrap busy=1.
      if(!getState().isFailed()) {
        Controller::worldPush();
        try {
          sm->search();
        } catch(EndOfSearch) {}
        Controller::worldPop();
        getState().setFailed(false);
      }
      WorkSteal::markIdle(*ctrl);
    }
    while(true) {
      WorkSteal::WorkItem item;
      if(!WorkSteal::popOrFinish(*ctrl, item))
        break;
      // popOrFinish has incremented busy and decremented idle on our
      // behalf. We now run a sub-tree.
      sm->reset();
      // Clear any residual failed flag from a prior sub-tree's terminal
      // state — search() requires a clean entry.
      getState().setFailed(false);
      // Bookend each work item with worldPush/worldPop so any
      // modifications that survive the search loop (the final
      // branch_right's right-side modify lands at the entry trail
      // level and is otherwise never undone) are cleaned up before
      // the worker takes its next item. Without this, leaked
      // modifications corrupt variable domains for subsequent replays
      // and cause solutions to disappear non-deterministically.
      //
      // Skip the worldPush+replay+search if isFailed is set —
      // worldPush asserts on a clean state. (Can happen if preprocess
      // already deduced UNSAT and a stale-failed flag survived from
      // earlier; just drop the work item, no exploration possible.)
      if(!getState().isFailed()) {
        Controller::worldPush();
        bool feasible = sm->replayPath(item);
        if(feasible) {
          try {
            if(!getState().isFailed())
              sm->search();
          } catch(EndOfSearch) {}
        } else {
          ctrl->replayFailures.fetch_add(1, std::memory_order_relaxed);
        }
        Controller::worldPop();
        getState().setFailed(false);
      }
      // Normal completion: search() returned because branches drained
      // to empty (sub-tree exhausted). For early termination via
      // EndOfSearch (timeout / stop), branches may still hold entries
      // and worldPushes are unmatched; that's fine for v1 because
      // stopRequested would be set, so popOrFinish will return false on
      // the next iteration and the worker exits without further replay.
      WorkSteal::markIdle(*ctrl);
    }
  } else {
    try {
      if(!getState().isFailed()) {
        sm->search();
      }
    } catch(EndOfSearch) {}
  }
  getState().setSearchManager(nullptr);

  if(getOptions().printonlyoptimal) {
    getOutput() << getState().storedSolution;
  }

  Parallel::endParallelMinion();
}
