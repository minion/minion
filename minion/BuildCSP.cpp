/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#include "minion.h"

#include "preprocess.h"

#include "search/search_control.h"

#include "search/SearchStrategies.h"
#include "search/neighbourhood-search.h"
#include "search/restartSearchManager.h"
#include "search/restartNewSearchManager.h"


#include "dump_state.hpp"

using namespace ProbSpec;
std::shared_ptr<NhConfig> makeNhConfig() {
  return make_shared<NhConfig>();
}


void BuildCSP(CSPInstance& instance) {
  getState().setTupleListContainer(instance.tupleListContainer);
  getState().setShortTupleListContainer(instance.shortTupleListContainer);

  // Set up variables
  BuildCon::build_variables(instance.vars);
  getState().setInstance(&instance);

  // Set up optimisation
  if(instance.is_optimisation_problem) {
    if(instance.optimise_minimising)
      Controller::optimise_minimise_var(
          BuildCon::get_AnyVarRef_from_Var(instance.optimise_variable));
    else
      Controller::optimise_maximise_var(
          BuildCon::get_AnyVarRef_from_Var(instance.optimise_variable));
  }

  vector<vector<AnyVarRef>>& print_matrix = getState().getPrintMatrix();

  // Reserve room in vector - no necessary but more efficent.
  print_matrix.reserve(instance.print_matrix.size());
  for(UnsignedSysInt i = 0; i < instance.print_matrix.size(); ++i)
    print_matrix.push_back(BuildCon::get_AnyVarRef_from_Var(instance.print_matrix[i]));

  // Impose Constraints
  for(list<ConstraintBlob>::iterator it = instance.constraints.begin();
      it != instance.constraints.end(); it++) {
    getState().addConstraint(build_constraint(*it));
  }

  // Solve!
  getState().getOldTimer().maybePrintTimestepStore(cout, "Setup Time: ", "SetupTime", getTableOut(),
                                                   !getOptions().silent);
  Controller::initalise_search();
  getState().getOldTimer().maybePrintTimestepStore(cout, "Initial Propagate: ", "InitialPropagate",
                                                   getTableOut(), !getOptions().silent);
}

void SolveCSP(CSPInstance& instance, SearchMethod args) {
  // Check that when searching PropagateSAC does actually do the SAC over all
  // vars in any
  // varorder block, not just the ones in the 'current' block

  // Set up variable and value ordering
  // Strange that when using randomise_valvarorder, the variables are
  // only shuffled within the VARORDER blocks from the input file.
  // Likewise, using a dynamic variable ordering, it only applies within
  // the VARORDER blocks.

  vector<AnyVarRef> preprocess_vars;

  for(SysInt i = (SysInt)instance.search_order.size() - 1; i >= 0; --i) {
    if(args.order != ORDER_NONE) {
      //  For each varorder block, overwrite with order given on the command
      //  line.
      instance.search_order[i].order = args.order;
      instance.search_order[i].limit = args.limit;
    }

    if(args.valorder != ValOrder(VALORDER_NONE)) {
      //  For each varorder block, overwrite with order given on the command
      //  line.
      for(UnsignedSysInt j = 0; j < instance.search_order[i].val_order.size(); ++j) {
        instance.search_order[i].val_order[j] = args.valorder;
      }
    }

    for(SysInt j = 0; j < (SysInt)instance.search_order[i].var_order.size();
        j++) { // cobble together all the varorder blocks for preprocessing.
      preprocess_vars.push_back(get_AnyVarRef_from_Var(instance.search_order[i].var_order[j]));
    }

    if(getOptions().randomise_valvarorder) {

      std::random_shuffle(instance.search_order[i].var_order.begin(),
                          instance.search_order[i].var_order.end());

      for(UnsignedSysInt j = 0; j < instance.search_order[i].val_order.size(); ++j) {
        instance.search_order[i].val_order[j] = VALORDER_RANDOM;
      }
    }

    D_ASSERT(instance.search_order[i].var_order.size() ==
             instance.search_order[i].val_order.size());
  }

  shared_ptr<Controller::SearchManager> sm;

  if(instance.neighbourhoodContainer) {

     sm = MakeNeighbourhoodSearch(args.prop_method, instance.search_order, *instance.neighbourhoodContainer);
  }
  else {
    if(getOptions().restarts) {
      if(getOptions().sollimit != 1) {
        D_FATAL_ERROR("-restarts is not compatible with -sollimit, or optimisation problems");
      }
      //sm = Controller::make_restart_search_manager(args.prop_method, instance.search_order);
      sm = Controller::make_restart_new_search_manager(args.prop_method, instance.search_order);
    }
    else {
      sm = Controller::make_search_manager(args.prop_method, instance.search_order);
    }
  }

  getState().getOldTimer().maybePrintTimestepStore(
      cout, "Build Search Ordering Time: ", "SearchOrderTime", getTableOut(), !getOptions().silent);
  try {

    try {
      PropogateCSP(std::max(args.preprocess, args.prop_method), preprocess_vars,
                   !getOptions().silent);
    } catch(EndOfSearch eos) {
      if(getOptions().outputCompressed != "" || getOptions().outputCompressedDomains)
        dump_solver(getOptions().outputCompressed, getOptions().outputCompressedDomains);
      throw;
    }

    getState().getOldTimer().maybePrintTimestepStore(cout, "Preprocess Time: ", "PreprocessTime",
                                                     getTableOut(), !getOptions().silent);

    if(getOptions().outputCompressed != "" || getOptions().outputCompressedDomains)
      dump_solver(getOptions().outputCompressed, getOptions().outputCompressedDomains);

    if(!getState().isFailed()) {
      sm->search();
    }
  } catch(EndOfSearch) {}

  if(getOptions().printonlyoptimal) {
    cout << getState().storedSolution;
  }

  getState().getOldTimer().maybePrintFinaltimestepStore(cout, "Solve Time: ", "SolveTime",
                                                        getTableOut(), !getOptions().silent);
  getOptions().printLine("Total Nodes: " + tostring(getState().getNodeCount()));
  getOptions().printLine(string("Problem solvable?: ") +
                         (getState().getSolutionCount() == 0 ? "no" : "yes"));


  getOptions().printLine("Solutions Found: " + tostring(getState().getSolutionCount()));

  getTableOut().set("Nodes", tostring(getState().getNodeCount()));
  getTableOut().set("Satisfiable", (getState().getSolutionCount() == 0 ? 0 : 1));
  getTableOut().set("SolutionsFound", getState().getSolutionCount());

  if(getOptions().tableout) {
    getTableOut().print_line(); // Outputs a line to the table file.
  }

#ifdef MORE_SEARCH_INFO
  if(!getOptions().silent)
    print_search_info();
#endif
}
