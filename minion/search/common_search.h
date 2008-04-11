/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/


#ifndef COMMON_SEARCH_H
#define COMMON_SEARCH_H

#include "../system/system.h"
#include "../solver.h"
#include "../variables/AnyVarRef.h"
#include "../variables/mappings/variable_neg.h"

#include "../CSPSpec.h"

namespace Controller
{
    
  
  /// Sets optimisation variable.
  template<typename VarRef>
	void optimise_maximise_var(StateObj* stateObj, VarRef var)
  {
	  getOptions(stateObj).findAllSolutions();
	  getState(stateObj).setOptimiseVar(new AnyVarRef(var));
	  getState(stateObj).setOptimisationProblem(true);
  }
  
  /// Sets optimisation variable.
  template<typename VarRef>
	void optimise_minimise_var(StateObj* stateObj, VarRef var)
  {
	  getOptions(stateObj).findAllSolutions();
	  getState(stateObj).setOptimiseVar(new AnyVarRef(VarNeg<VarRef>(var)));
	  getState(stateObj).setOptimisationProblem(true);
  }
  
  /// Ensures a particular constraint is satisfied by the solution.
  template<typename T>
	void check_constraint(StateObj* stateObj, T* con)
  {
	  vector<AnyVarRef> variables = con->get_vars();
	  unsigned vec_size = variables.size();	  
	  vector<DomainInt> values(vec_size);

	  for(unsigned loop = 0; loop < vec_size; ++loop)
	  {
		if(!variables[loop].isAssigned())
		{
		  cerr << "Some variables are unassigned. Unless you purposefully " <<
		  "left them out, have a look." << endl;
		  return;
		}
		values[loop] = variables[loop].getAssignedValue();
	  }
	  
	  if(!con->check_assignment(values))
	  {
	    cerr << "A " << con->constraint_name() << " constraint is not satisfied by this sol!" << endl;
		cerr << "The constraint is over the following variables:" << endl;
		for(unsigned loop = 0; loop < vec_size; ++loop)
		  cerr << variables[loop] << ",";
		cerr << endl;
		cerr << "Variables were assigned:" << endl;
	    for(unsigned loop = 0; loop < vec_size; ++loop)
		  cerr << values[loop] << ",";
		cerr << endl;
		cerr << "This is an internal bug. It shouldn't happen!!" << endl;
		cerr << "Please report this instance to the developers." << endl;
		FAIL_EXIT();
	  }
  }

  /// All operations to be performed when a solution is found.
  /// This function checks the solution is correct, and prints it if required.
  inline void check_sol_is_correct(StateObj* stateObj)
  {
    //if(solution_check != NULL)
	//  solution_check();
	getState(stateObj).incrementSolutionCount();
    if(getOptions(stateObj).solsoutWrite)
    {
      for(unsigned i = 0; i < print_matrix.size(); ++i)
        for(unsigned j = 0; j < print_matrix.size(); ++j)
        {
          if(!print_matrix[i][j].isAssigned())
            D_FATAL_ERROR("Some variable was unassigned while writing solution to file.");
          solsoutFile << print_matrix[i][j].getAssignedValue() << " ";
        }
      solsoutFile << "\n";
    }
    
	if(getOptions(stateObj).print_solution)
	{
	  if(!print_matrix.empty())
	  {
		for(unsigned i = 0; i < print_matrix.size(); ++i)
		{
	          if (!getOptions(stateObj).print_only_solution) cout << "Sol: ";  
		  for(unsigned j = 0; j < print_matrix[i].size(); ++j)
		  {
			if(!print_matrix[i][j].isAssigned())
			  cout  << "[" << print_matrix[i][j].getMin() << "," << 
			                 print_matrix[i][j].getMax() << "]";
			else
			  cout << print_matrix[i][j].getAssignedValue() << " ";
		  }
		  cout << endl;
		}
		if (!getOptions(stateObj).print_only_solution) cout << endl;
	  }
  
	  // TODO : Make this more easily changable.
          if (!getOptions(stateObj).print_only_solution) 
          {
	    cout << "Solution Number: " << getState(stateObj).getSolutionCount() << endl;
	    getState(stateObj).getTimer().printTimestepWithoutReset("Time:");
	    cout << "Nodes: " << getState(stateObj).getNodeCount() << endl << endl;
          }
    }

  if(!getOptions(stateObj).nocheck)
  {
    for(unsigned i = 0; i < getState(stateObj).getDynamicConstraintList().size(); ++i)
      check_constraint(stateObj, getState(stateObj).getDynamicConstraintList()[i]);
  
    for(unsigned i = 0 ; i < getState(stateObj).getConstraintList().size();i++)
      check_constraint(stateObj, getState(stateObj).getConstraintList()[i]);
  }
  }
  
   
  /// Check if timelimit has been exceeded.
  inline bool do_checks(StateObj* stateObj)
  {
  	if(getState(stateObj).isAlarmActivated())
  	{
      getState(stateObj).clearAlarm();
      if(getState(stateObj).isCtrlcPressed())
        return true;

  	  if(getOptions(stateObj).time_limit != 0)
  	  {
  	    if(getState(stateObj).getTimer().checkTimeout(getOptions(stateObj).time_limit))
  	    {
  		  cout << "Time out." << endl;
            tableout.set("TimeOut", 1);
  		  return true;
  	    }
  	  }
  	}
	return false;
  }
  
  
template<typename T>
void inline maybe_print_search_state(StateObj* stateObj, const char* name, T& vars)
{
  if(getOptions(stateObj).dumptree)
	cout << name << getState(stateObj).getNodeCount() << "," << get_dom_as_string(vars) << endl;
}

void inline maybe_print_search_action(StateObj* stateObj, const char* action)
{
    // used to print "bt" usually
    if(getOptions(stateObj).dumptree)
        cout << "SearchAction:" << action << endl;
}

  void inline deal_with_solution(StateObj* stateObj)
  {
	if(getState(stateObj).isOptimisationProblem())
	{
	  if(!getState(stateObj).getOptimiseVar()->isAssigned())
	  {
		cerr << "The optimisation variable isn't assigned at a solution node!" << endl;
		cerr << "Put it in the variable ordering?" << endl;
		FAIL_EXIT();
	  }
	  
	  cout << "Solution found with Value: " 
	  << getState(stateObj).getOptimiseVar()->getAssignedValue() << endl;
	  
	  getState(stateObj).setOptimiseValue(getState(stateObj).getOptimiseVar()->getAssignedValue() + 1);			
	}
    // Note that sollimit = -1 if all solutions should be found.
	if(getState(stateObj).getSolutionCount() == getOptions(stateObj).sollimit)
	  throw 0;
  }

  void inline set_optimise_and_propagate_queue(StateObj* stateObj)
  {
  #ifdef USE_SETJMP
	if(getState(stateObj).isOptimisationProblem())
	{
	  // Must check if this setMin will fail before doing it, else
	  // The setjmp will throw us off. It's cheaper to check than set up
	  // a new setjmp point here.
	  if(getState(stateObj).getOptimiseVar()->getMax() >= getState(stateObj).getOptimiseValue())
	  { 
		getState(stateObj).getOptimiseVar()->setMin(getState(stateObj).getOptimiseValue());
		getQueue(stateObj).propagateQueue();
	  }
	  else
	  {failed = true; }
	}
	else
	{ getQueue(stateObj).propagateQueue();}
  #else
	if(getState(stateObj).isOptimisationProblem())
	  getState(stateObj).getOptimiseVar()->setMin(getState(stateObj).getOptimiseValue());
	getQueue(stateObj).propagateQueue();
  #endif	
  }

  void inline initalise_search(StateObj* stateObj)
  {
	getState(stateObj).setSolutionCount(0);  
	getState(stateObj).setNodeCount(0);
  getState(stateObj).setupAlarm();
  install_ctrlc_trigger(stateObj);
	lock(stateObj);
	getState(stateObj).getTimer().printTimestepWithoutReset("First Node Time: ");
	/// Failed initially propagating constraints!
	if(getState(stateObj).isFailed())
	  return;
	if(getState(stateObj).isOptimisationProblem())
	  getState(stateObj).setOptimiseValue(getState(stateObj).getOptimiseVar()->getMin()); 
  }
}

#endif
