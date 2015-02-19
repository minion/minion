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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "minion.h"

#include "preprocess.h"

#include "search/search_control.h"


#include "dump_state.hpp"



using namespace ProbSpec;

/// Builds the CSP given by instance into stateObj.
void BuildCSP(StateObj* stateObj, CSPInstance& instance)
{
  getState(stateObj).setTupleListContainer(instance.tupleListContainer);
  getState(stateObj).setShortTupleListContainer(instance.shortTupleListContainer);

  // XXX : Hack for reify / reifyimply problem.
  getState(stateObj).setDynamicTriggersUsed(true);

  // Set up variables
  BuildCon::build_variables(stateObj, instance.vars);
  getState(stateObj).setInstance(&instance);

  // Set up optimisation
  if(instance.is_optimisation_problem)
  {
    if(instance.optimise_minimising)
      Controller::optimise_minimise_var(stateObj, BuildCon::get_AnyVarRef_from_Var(stateObj, instance.optimise_variable));
    else
      Controller::optimise_maximise_var(stateObj, BuildCon::get_AnyVarRef_from_Var(stateObj, instance.optimise_variable));
  }

  vector<vector<AnyVarRef> >& print_matrix = getState(stateObj).getPrintMatrix();

  // Reserve room in vector - no necessary but more efficent.
  print_matrix.reserve(instance.print_matrix.size());
  for(UnsignedSysInt i = 0; i < instance.print_matrix.size(); ++i)
      print_matrix.push_back(BuildCon::get_AnyVarRef_from_Var(stateObj, instance.print_matrix[i]));

  // Impose Constraints
  for(list<ConstraintBlob>::iterator it = instance.constraints.begin(); it != instance.constraints.end(); it++) {
     getState(stateObj).addConstraint(build_constraint(stateObj, *it));
  }

  // Solve!
  getState(stateObj).getOldTimer().maybePrintTimestepStore(cout, Output_Always, "Setup Time: ", "SetupTime", getTableOut(), !getOptions(stateObj).silent);
  Controller::initalise_search(stateObj);
  getState(stateObj).getOldTimer().maybePrintTimestepStore(cout, Output_Always, "Initial Propagate: ", "InitialPropagate", getTableOut(), !getOptions(stateObj).silent);

}

void SolveCSP(StateObj* stateObj, CSPInstance& instance, SearchMethod args)
{
    // Check that when searching PropagateSAC does actually do the SAC over all vars in any
    // varorder block, not just the ones in the 'current' block


    // Set up variable and value ordering
    // Strange that when using randomise_valvarorder, the variables are
    // only shuffled within the VARORDER blocks from the input file.
    // Likewise, using a dynamic variable ordering, it only applies within
    // the VARORDER blocks.

    vector<AnyVarRef> preprocess_vars;

    for(SysInt i = (SysInt)instance.search_order.size() - 1; i >= 0; --i)
    {
        if(args.order != ORDER_NONE) {
            //  For each varorder block, overwrite with order given on the command line.
            instance.search_order[i].order = args.order;
            instance.search_order[i].limit = args.limit;
        }
        
        for(SysInt j=0; j<(SysInt)instance.search_order[i].var_order.size(); j++)
        {   // cobble together all the varorder blocks for preprocessing.
            preprocess_vars.push_back(get_AnyVarRef_from_Var(stateObj, instance.search_order[i].var_order[j]));
        }

        if(getOptions(stateObj).randomise_valvarorder)
        {
            getOptions(stateObj).printLine("Using seed: " + tostring(args.random_seed));
            srand( args.random_seed );

            std::random_shuffle(instance.search_order[i].var_order.begin(), instance.search_order[i].var_order.end());

            for(UnsignedSysInt j = 0; j < instance.search_order[i].val_order.size(); ++j)
            {
              instance.search_order[i].val_order[j] = VALORDER_RANDOM;
            }
        }

        D_ASSERT(instance.search_order[i].var_order.size() == instance.search_order[i].val_order.size());
    }

    shared_ptr<Controller::SearchManager> sm=Controller::make_search_manager(stateObj, args.prop_method, instance.search_order);

    getState(stateObj).getOldTimer().maybePrintTimestepStore(cout, Output_2, "Build Search Ordering Time: ", "SearchOrderTime", getTableOut(), !getOptions(stateObj).silent);
    try {

      try {
      PropogateCSP(stateObj, std::max(args.preprocess, args.prop_method), preprocess_vars, !getOptions(stateObj).silent);
      }
      catch(EndOfSearch eos)
      {
          if(getOptions(stateObj).outputCompressed != "" || getOptions(stateObj).outputCompressedDomains)
              dump_solver(stateObj, getOptions(stateObj).outputCompressed, getOptions(stateObj).outputCompressedDomains);
          throw;
      }

      getState(stateObj).getOldTimer().maybePrintTimestepStore(cout, Output_2, "Preprocess Time: ", "PreprocessTime", getTableOut(), !getOptions(stateObj).silent);
      getState(stateObj).getOldTimer().maybePrintTimestepStore(cout, Output_1, "First node time: ", "FirstNodeTime", getTableOut(), !getOptions(stateObj).silent);

      if(getOptions(stateObj).outputCompressed != "" || getOptions(stateObj).outputCompressedDomains)
        dump_solver(stateObj, getOptions(stateObj).outputCompressed, getOptions(stateObj).outputCompressedDomains);

      if(!getState(stateObj).isFailed())
      {
        sm->search();
      }
    }
    catch(EndOfSearch)
    { }

  if(getOptions(stateObj).printonlyoptimal)
  {
    cout << getState(stateObj).storedSolution;
  }

  getState(stateObj).getOldTimer().maybePrintFinaltimestepStore(cout, "Solve Time: ", "SolveTime", getTableOut(), !getOptions(stateObj).silent);
  getOptions(stateObj).printLine("Total Nodes: " + tostring( getState(stateObj).getNodeCount() ));
  getOptions(stateObj).printLine(string("Problem solvable?: ") + (getState(stateObj).getSolutionCount() == 0 ? "no" : "yes"));

  if(getOptions(stateObj).cspcomp)
  {
    if(getState(stateObj).getSolutionCount() != 0)
      cout << "s SATISFIABLE" << endl;
    else
      cout << "s UNSATISFIABLE" << endl;
  }

  getOptions(stateObj).printLine("Solutions Found: " + tostring(getState(stateObj).getSolutionCount()));

  getTableOut().set("Nodes", tostring(getState(stateObj).getNodeCount()));
  getTableOut().set("Satisfiable", (getState(stateObj).getSolutionCount()==0 ? 0 : 1));
  getTableOut().set("SolutionsFound", getState(stateObj).getSolutionCount());

  if(getOptions(stateObj).tableout)
  {
    getTableOut().print_line();  // Outputs a line to the table file.
  }

#ifdef MORE_SEARCH_INFO
  if(!getOptions(stateObj).silent)
    print_search_info();
#endif

}
