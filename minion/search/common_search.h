/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/


#ifndef COMMON_SEARCH_H
#define COMMON_SEARCH_H

namespace Controller
{
   /// This variable contains the name of a function which should be called
  /// Wherever a solution is found.
  VARDEF_ASSIGN(void (*_solution_check)(void), NULL);
  
  /// Sets the function to be called when a solution is found.
  inline void set_solution_check_function(void(*fun_ptr)(void))
  { _solution_check = fun_ptr; }

  /// Global variable to denote if only one solution should be found.
  VARDEF_ASSIGN(BOOL _find_one_sol,true);
  
  /// Global variable to denote if solutions should be printed.
  VARDEF_ASSIGN(BOOL print_solution, true);
  

  
  /// Makes solver find all solutions.
  inline void find_all_solutions()
  { _find_one_sol = false; }
  
  /// Sets optimisation variable.
  template<typename VarRef>
	void optimise_maximise_var(VarRef var)
  {
	  _find_one_sol = false;
	  optimise_var = new AnyVarRef(var);
	  optimise = true;
  }
  
  /// Sets optimisation variable.
  template<typename VarRef>
	void optimise_minimise_var(VarRef var)
  {
	  _find_one_sol = false;
	  optimise_var = new AnyVarRef(VarNeg<VarRef>(var));
	  optimise = true;
  }
  
  /// Ensures a particular constraint is satisfied by the solution.
  template<typename T>
	void check_constraint(T* con)
  {
	  vector<AnyVarRef> variables = con->get_vars();
	  unsigned vec_size = variables.size();	  
	  vector<int> values(vec_size);

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
		  cerr << string(variables[loop]) << ",";
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
  inline void check_sol_is_correct()
  {
    if(_solution_check != NULL)
	  _solution_check();
	++solutions;
	if(print_solution)
	{
	  if(!print_matrix.empty())
	  {
		for(unsigned i = 0; i < print_matrix.size(); ++i)
		{
	          if (!Controller::print_only_solution) cout << "Sol: ";  
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
		if (!Controller::print_only_solution) cout << endl;
	  }
  
	  // TODO : Make this more easily changable.
          if (!Controller::print_only_solution) 
          {
	    cout << "Solution Number: " << solutions << endl;
	    print_timestep_without_reset("Time:");
	    cout << "Nodes: " << nodes << endl << endl;
          }
    }
#ifndef NO_DEBUG
  if(!commandlineoption_nocheck)
  {
    for(unsigned i = 0; i < dynamic_constraints.size(); ++i)
      check_constraint(dynamic_constraints[i]);
  
    for(unsigned i = 0 ; i < constraints.size();i++)
      check_constraint(constraints[i]);
  }
#endif
  }
  
   
  /// Check if timelimit has been exceeded.
  inline BOOL do_checks()
  {
  	if((nodes & 1023) == 0)
	{
	  if(time_limit != 0)
	  {
	    if(check_timeout(time_limit))
	    {
		  cout << "Time out." << endl;
		  return true;
	    }
	  }
	}
	return false;
  }
  
  
#ifdef NO_DEBUG
#define maybe_print_search_state(x,y)
#else
template<typename T>
void maybe_print_search_state(string name, T& vars)
{
  if(commandlineoption_dumptree)
	cout << name << nodes << "," << get_dom_as_string(vars) << endl;
}
#endif

  void inline deal_with_solution()
  {
	if(optimise)
	{
	  if(!optimise_var->isAssigned())
	  {
		cerr << "The optimisation variable isn't assigned at a solution node!" << endl;
		cerr << "Put it in the variable ordering?" << endl;
		FAIL_EXIT();
	  }
	  
	  cout << "Solution found with Value: " 
	  << optimise_var->getAssignedValue() << endl;
	  
	  current_optimise_position = optimise_var->getAssignedValue() + 1;			
	}
	if(_find_one_sol || solutions == commandlineoption_sollimit)
	  throw 0;
  }

  void inline set_optimise_and_propogate_queue()
  {
  #ifdef USE_SETJMP
	if(optimise)
	{
	  // Must check if this setMin will fail before doing it, else
	  // The setjmp will throw us off. It's cheaper to check than set up
	  // a new setjmp point here.
	  if(optimise_var->getMax() >= current_optimise_position)
	  { 
		optimise_var->setMin(current_optimise_position);
		propogate_queue();
	  }
	  else
	  {failed = true; }
	}
	else
	{ propogate_queue();}
  #else
	if(optimise)
	  optimise_var->setMin(current_optimise_position);
	propogate_queue();
  #endif	
  }

}

#endif
