/*
 *  BuildCSP.cpp
 *  Minion
 *
 *
 */

#include "minion.h"

#include "search/standard_search.h"
#include "search/recursive_search.h"
#include "search/conflict_search.h"
#include "search/group_search.h"

#include "search/search_control.h"

#include "CSPSpec.h"


using namespace ProbSpec;

/// Builds the CSP given by instance into stateObj.
void BuildCSP(StateObj* stateObj, CSPInstance& instance)
{
  // Set up variables
  BuildCon::build_variables(stateObj, instance.vars);
  
  // Horrible hack 
  getMemory(stateObj).monotonic_set = & getVars(stateObj).getBigRangevarContainer().bms_array;
  
  // Set up optimisation
  if(instance.is_optimisation_problem)
  {
    if(instance.optimise_minimising)
      Controller::optimise_minimise_var(stateObj, BuildCon::get_AnyVarRef_from_Var(stateObj, instance.optimise_variable));
	else
	  Controller::optimise_maximise_var(stateObj, BuildCon::get_AnyVarRef_from_Var(stateObj, instance.optimise_variable));
  }
  
  // Set up printing
  Controller::print_matrix.resize(instance.print_matrix.size());
  for(unsigned i = 0; i < instance.print_matrix.size(); ++i)
  {
    for(unsigned j = 0; j < instance.print_matrix[i].size(); ++j)
	  Controller::print_matrix[i].push_back(BuildCon::get_AnyVarRef_from_Var(stateObj, instance.print_matrix[i][j]));
  }
  
  // Impose Constraints
  for(list<ConstraintBlob>::iterator it = instance.constraints.begin();
	  it != instance.constraints.end(); ++it)
  {
    if(it->is_dynamic())
    {
#ifdef DYNAMICTRIGGERS
      getState(stateObj).addConstraint(build_dynamic_constraint(stateObj, *it));
      getState(stateObj).setDynamicTriggersUsed(true);
#else
      cout << "Sorry, cannot process this constraint as it needs dynamic triggers or watched literals." << endl ;
      cout << "use an alternative encoding or recompile with -DWATCHEDLITERALS or -DDYNAMICTRIGGERS in command line" << endl;
      exit(1);
#endif
    }
    else
      getState(stateObj).addConstraint(build_constraint(stateObj, *it));
  }
  
  // Solve!
  getState(stateObj).getOldTimer().maybePrintTimestepStore("Setup Time: ", "SetupTime", oldtableout, !getOptions(stateObj).print_only_solution);
  Controller::initalise_search(stateObj);
  getState(stateObj).getOldTimer().maybePrintTimestepStore("Initial Propagate: ", "InitialPropagate", oldtableout, !getOptions(stateObj).print_only_solution);
  
}

void SolveCSP(StateObj* stateObj, CSPInstance& instance, MinionArguments args)
{
  // Set up variable and value ordering
  pair<vector<AnyVarRef>, vector<int> > var_val_order = BuildCon::build_val_and_var_order(stateObj, instance);
  
  if(getOptions(stateObj).randomise_valvarorder)
  {
    getOptions(stateObj).printLine("Using seed: " + args.random_seed);
    srand( args.random_seed );
    
    std::random_shuffle(var_val_order.first.begin(), var_val_order.first.end());
    for(unsigned i = 0; i < var_val_order.second.size(); ++i)
      var_val_order.second[i] = (rand() % 100) > 50;
  }
  
  if(!getState(stateObj).isFailed())
  {
    PropogateCSP(stateObj, args.preprocess, var_val_order.first, !getOptions(stateObj).print_only_solution);
	  getState(stateObj).getOldTimer().maybePrintTimestepStore("First node time: ", "FirstNodeTime", oldtableout, !getOptions(stateObj).print_only_solution);
	  if(!getState(stateObj).isFailed())
    {
      switch(args.prop_method)
      {
        case PropLevel_GAC:
          solve(stateObj, args.order, var_val_order, instance, PropagateGAC());   // add a getState(stateObj).getOldTimer().maybePrintTimestepStore to search..
          break;
        case PropLevel_SAC:
          solve(stateObj, args.order, var_val_order, instance, PropagateSAC());
          break;
        case PropLevel_SSAC:
          solve(stateObj, args.order, var_val_order, instance, PropagateSSAC());
          break;
        default:
          abort();
      }
    }
  }
  else
  {
    getState(stateObj).getOldTimer().maybePrintTimestepStore("First node time: ", "FirstNodeTime", oldtableout, !getOptions(stateObj).print_only_solution);
  }
  
  getState(stateObj).getOldTimer().maybePrintFinaltimestepStore("Solve Time: ", "SolveTime", oldtableout, !getOptions(stateObj).print_only_solution);
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
  
  oldtableout.set("Nodes", to_string(getState(stateObj).getNodeCount()));
  oldtableout.set("Satisfiable", (getState(stateObj).getSolutionCount()==0 ? 0 : 1));
  oldtableout.set("SolutionsFound", getState(stateObj).getSolutionCount());
  
  if(getOptions(stateObj).tableout)
  {
    oldtableout.print_line();  // Outputs a line to the table file.
  }
  
#ifdef MORE_SEARCH_INFO
  if(!getOptions(stateObj).cspcomp)
    print_search_info();
#endif
  
}
