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

#include "search/standard_search.h"
#include "search/recursive_search.h"
#include "search/conflict_search.h"
#include "search/group_search.h"

#include "search/search_control.h"

#include "CSPSpec.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

using boost::function;
using boost::bind;


using namespace ProbSpec;

/// Builds the CSP given by instance into stateObj.
void BuildCSP(StateObj* stateObj, CSPInstance& instance)
{
  // XXX : Hack for reify / reifyimply problem.
  getState(stateObj).setDynamicTriggersUsed(true);
        
  // Set up variables
  BuildCon::build_variables(stateObj, instance.vars);
  
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
  for(list<ConstraintBlob>::iterator it = instance.constraints.begin();
      it != instance.constraints.end(); ++it)
  {
     getState(stateObj).addConstraint(build_constraint(stateObj, *it));
  }
  
  // Solve!
  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_Always, "Setup Time: ", "SetupTime", getTableOut(), !getOptions(stateObj).silent);
  Controller::initalise_search(stateObj);
  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_Always, "Initial Propagate: ", "InitialPropagate", getTableOut(), !getOptions(stateObj).silent);
  
}

void SolveCSP(StateObj* stateObj, CSPInstance& instance, MinionArguments args)
{
  vector<AnyVarRef> preprocess_vars = BuildCon::build_val_and_var_order(stateObj, instance.search_order[0]).first;  
  function<void (void)> search(bind(Controller::deal_with_solution, stateObj));
      
  // Set up variable and value ordering
  for(int i = instance.search_order.size() - 1; i >= 0; --i)
  {
    SearchOrder order = instance.search_order[i];
    
    if(args.order != ORDER_NONE)
      order.order = args.order;
      
    pair<vector<AnyVarRef>, vector<int> > var_val_order = BuildCon::build_val_and_var_order(stateObj, instance.search_order[i]);

    if(getOptions(stateObj).randomise_valvarorder)
    {
      getOptions(stateObj).printLine("Using seed: " + to_string(args.random_seed));
      srand( args.random_seed );

      std::random_shuffle(var_val_order.first.begin(), var_val_order.first.end());
      for(unsigned i = 0; i < var_val_order.second.size(); ++i)
        var_val_order.second[i] = (rand() % 100) > 50;
    }
    
    switch(args.prop_method)
    {
      case PropLevel_GAC:
      search = solve(stateObj, search, order, var_val_order, instance, PropagateGAC());
      break;
      case PropLevel_SAC:
      search = solve(stateObj, search, order, var_val_order, instance, PropagateSAC());
      break;
      case PropLevel_SSAC:
      search = solve(stateObj, search, order, var_val_order, instance, PropagateSSAC());
      break;
      default:
      abort();
    }
  }

  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_2, "Build Search Ordering Time: ", "SearchOrderTime", getTableOut(), !getOptions(stateObj).silent);

  PropogateCSP(stateObj, args.preprocess, preprocess_vars, !getOptions(stateObj).silent);
  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_2, "Preprocess Time: ", "PreprocessTime", getTableOut(), !getOptions(stateObj).silent);
  getState(stateObj).getOldTimer().maybePrintTimestepStore(Output_1, "First node time: ", "FirstNodeTime", getTableOut(), !getOptions(stateObj).silent);
  
  if(!getState(stateObj).isFailed())
  {
    try
      { search(); }
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
