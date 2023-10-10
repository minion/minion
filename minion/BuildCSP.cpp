// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "parallel/parallel.h"
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

      std::random_shuffle(instance.searchOrder[i].varOrder.begin(),
                          instance.searchOrder[i].varOrder.end());

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
  getState().getOldTimer().maybePrintTimestepStore(cout, "Setup Time: ", "SetupTime", getTableOut(),
                                                   !getOptions().silent);
  Controller::initalise_search();
  getState().getOldTimer().maybePrintTimestepStore(cout, "Initial Propagate: ", "InitialPropagate",
                                                   getTableOut(), !getOptions().silent);
}

bool PreprocessCSP(CSPInstance& instance, SearchMethod args) {
    vector<AnyVarRef> preprocess_anyvars = getAnyVarRefFromVar(instance.preprocess_vars);

    try {
      PropogateCSP(std::max(args.preprocess, args.propMethod), preprocess_anyvars,
                   !getOptions().silent);
    } catch(EndOfSearch eos) {
      return false;
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
    if(getOptions().sollimit != 1) {
      D_FATAL_ERROR("-restarts is not compatible with -sollimit, or optimisation problems");
    }
    // sm = Controller::make_restart_search_manager(args.propMethod, instance.searchOrder);
    sm = Controller::make_restart_new_search_manager(args.propMethod, instance.searchOrder);
  } else {
    sm = Controller::makeSearch_manager(args.propMethod, instance.searchOrder);
  }

  try {
    if(!getState().isFailed()) {
      sm->search();
    }
  } catch(EndOfSearch) {}

  if(getOptions().printonlyoptimal) {
    cout << getState().storedSolution;
  }

  Parallel::endParallelMinion();
}
