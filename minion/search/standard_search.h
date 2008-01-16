/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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

namespace Controller
{

#include "VariableOrders.h"
  
  VARDEF_ASSIGN(void (*_solution_check)(void), NULL);
  
  inline void set_solution_check_function(void(*fun_ptr)(void))
  { _solution_check = fun_ptr; }

  VARDEF_ASSIGN(BOOL _find_one_sol,true);
  VARDEF_ASSIGN(BOOL print_solution, true);
  
  inline void find_all_solutions()
  { _find_one_sol = false; }
  
  template<typename VarRef>
	void optimise_maximise_var(VarRef var)
  {
	  _find_one_sol = false;
	  optimise_var = new AnyVarRef(var);
	  optimise = true;
  }
  
  template<typename VarRef>
	void optimise_minimise_var(VarRef var)
  {
	  _find_one_sol = false;
	  optimise_var = new AnyVarRef(VarNeg<VarRef>(var));
	  optimise = true;
  }
  
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


  inline void check_sol()
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
		  cout << "Sol: ";  
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
		cout << endl;
	  }
  
	  // TODO : Make this more easily changable.
	  cout << "Solution Number: " << solutions << endl;
	  print_timestep_without_reset("Time:");
	  cout << "Nodes: " << nodes << endl << endl;
    }
#ifdef MORE_SEARCH_INFO
  if(!commandlineoption_nocheck)
  {
    for(unsigned i = 0; i < dynamic_constraints.size(); ++i)
      check_constraint(dynamic_constraints[i]);
  
    for(unsigned i = 0 ; i < constraints.size();i++)
      check_constraint(constraints[i]);
  }
#endif
  }
  
  
  inline BOOL do_checks()
  {
	if(time_limit != 0)
	{
	  if(check_timeout(time_limit))
	  {
		cout << "Time out." << endl;
		return true;
	  }
	}
	return false;
  }
  
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)
  // find_next_unassigned() : gets 

  template<typename VarOrder, typename Variables>
	inline void solve_loop(VarOrder& order, Variables& v)
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
#ifdef MORE_SEARCH_INFO
	  if(commandlineoption_dumptree)
		cout << "Node: " << nodes << "," << get_dom_as_string(v) << endl;
#endif
	  
	  while(true)
	  {
		nodes++;
#ifdef MORE_SEARCH_INFO
		if(nodes == commandlineoption_nodelimit)
		  return;
		if((nodes & 1023) == 0)
		{
		  if(do_checks())
			return;
		}
#endif
		
		// order.find_next_unassigned returns true if all variables assigned.
		if(order.find_next_unassigned())
		{  
		  
#ifdef MORE_SEARCH_INFO
		  if(commandlineoption_dumptree)
			cout << "Sol: " << nodes << "," << get_dom_as_string(v) << endl;
#endif
		  
		  // We have found a solution!
		  check_sol();
		  if(optimise)
		  {
			
#ifdef MORE_SEARCH_INFO
		    if(!optimise_var->isAssigned())
			{
			  cerr << "The optimisation variable isn't assigned at a solution node!" << endl;
			  cerr << "Put it in the variable ordering?" << endl;
			  FAIL_EXIT();
			}
#endif
			
			cout << "Solution found with Value: " 
			<< optimise_var->getAssignedValue() << endl;
			
			current_optimise_position = optimise_var->getAssignedValue() + 1;
			cout << "New optimisation Value: " << current_optimise_position << endl;
			
		  }
		  if(_find_one_sol || solutions == commandlineoption_sollimit)
			return;
		  
	
		  // fail here to force backtracking.
		  failed = true;
		}
		else
		{
		  D_ASSERT(order.cur_var_not_assigned());
#ifdef MORE_SEARCH_INFO
		  if(commandlineoption_dumptree)
			cout << "Node: " << nodes << "," << get_dom_as_string(v) << endl;
#endif
		  world_push();
		  order.branch_left();
		  propogate_queue();
		}
		
		while(failed)
		{
		  failed = false;
		  
		  if(order.finished_search())
			return;
#ifdef MORE_SEARCH_INFO
		  if(commandlineoption_dumptree)
			cout << "Node: " << nodes << "," << get_dom_as_string(v) << endl;
#endif
		  world_pop();
		  order.branch_right();
		  if(optimise)
			optimise_var->setMin(current_optimise_position);
		  propogate_queue();
		}
		
	  }
  }
  

  template<typename VarOrder, typename VarArray>
  inline void solve(VarOrder& order, VarArray& vars)
  {
	  solutions = 0;  
	  nodes = 0;
	  lock();
	  print_timestep_without_reset("First node time: ");
	  /// Failed initially propagating constraints!
	  if(Controller::failed)
		return;
	  if(optimise)
		current_optimise_position = optimise_var->getMin(); 
	  solve_loop(order, vars);
  }
}




