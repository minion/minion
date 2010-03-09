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

//#include "search/standard_search.h"
//#include "search/recursive_search.h"
//#include "search/conflict_search.h"
//#include "search/group_search.h"

#include "search/search_control.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

using boost::function;
using boost::bind;


using namespace ProbSpec;

/// Builds the CSP given by instance into stateObj.
void BuildCSP(StateObj* stateObj, CSPInstance& instance)
{
  getState(stateObj).setTupleListContainer(instance.tupleListContainer);

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
  for(unsigned i = 0; i < instance.print_matrix.size(); ++i)
      print_matrix.push_back(BuildCon::get_AnyVarRef_from_Var(stateObj, instance.print_matrix[i]));

  // Impose Constraints
  while(!instance.constraints.empty())
  {
     getState(stateObj).addConstraint(build_constraint(stateObj, instance.constraints.front()));
     instance.constraints.pop_front();
  }
  
  // Solve!
  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_Always, "Setup Time: ", "SetupTime", getTableOut(), !getOptions(stateObj).silent);
  Controller::initalise_search(stateObj);
  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_Always, "Initial Propagate: ", "InitialPropagate", getTableOut(), !getOptions(stateObj).silent);

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
    
    for(int i = instance.search_order.size() - 1; i >= 0; --i)
    {
        if(args.order != ORDER_NONE)
            instance.search_order[i].order = args.order;
        
        for(int j=0; j<instance.search_order[i].var_order.size(); j++)
        {   // cobble together all the varorder blocks for preprocessing.
            preprocess_vars.push_back(get_AnyVarRef_from_Var(stateObj, instance.search_order[i].var_order[j]));
        }
        
        if(getOptions(stateObj).randomise_valvarorder)
        {
            getOptions(stateObj).printLine("Using seed: " + to_string(args.random_seed));
            srand( args.random_seed );
            
            std::random_shuffle(instance.search_order[i].var_order.begin(), instance.search_order[i].var_order.end()); 
            
            for(unsigned j = 0; j < instance.search_order[i].val_order.size(); ++j)
                instance.search_order[i].val_order[j] = (rand() % 100) > 50;
        }
    }
    
    shared_ptr<Controller::SearchManager> sm=Controller::make_search_manager(stateObj, args.prop_method, instance.search_order);
    
    getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_2, "Build Search Ordering Time: ", "SearchOrderTime", getTableOut(), !getOptions(stateObj).silent);
    
    PropogateCSP(stateObj, args.preprocess, preprocess_vars, !getOptions(stateObj).silent);
    getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_2, "Preprocess Time: ", "PreprocessTime", getTableOut(), !getOptions(stateObj).silent);
    getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_1, "First node time: ", "FirstNodeTime", getTableOut(), !getOptions(stateObj).silent);
    
  if(!getState(stateObj).isFailed())
  {
    try {
        if(!getOptions(stateObj).noTimers && getOptions(stateObj).search_limit > 0)
        {
            getState(stateObj).setupAlarm(getOptions(stateObj).timeout_active, getOptions(stateObj).search_limit, getOptions(stateObj).time_limit_is_CPU_time);
            getState(stateObj).setupCtrlc();
        }
        sm->search();
    }
    catch(EndOfSearch)
    { }
  }

  getState(stateObj).getOldTimer().maybePrintFinaltimestepStore("Solve Time: ", "SolveTime", getTableOut(), !getOptions(stateObj).silent);
  getOptions(stateObj).printLine("Total Nodes: " + to_string( getState(stateObj).getNodeCount() ));
  getOptions(stateObj).printLine(string("Problem solvable?: ") + (getState(stateObj).getSolutionCount() == 0 ? "no" : "yes"));

  if(getOptions(stateObj).cspcomp)
  {
    if(getState(stateObj).getSolutionCount() != 0)
      cout << "s SATISFIABLE" << endl;
    else
      cout << "s UNSATISFIABLE" << endl;
  }

  getOptions(stateObj).printLine("Solutions Found: " + to_string(getState(stateObj).getSolutionCount()));

  getTableOut().set("Nodes", to_string(getState(stateObj).getNodeCount()));
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
