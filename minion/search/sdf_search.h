/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_search.h 478 2006-11-24 09:42:10Z azumanga $
*/

#include "common_search.h"

namespace Controller
{
  // Returns smallest domain, or -1 if all variables assigned.
  template<typename Variables>
  int find_smallest_domain(Variables& order)
  {
	int smallest_dom = -1;
	int dom_size = numeric_limits<int>::max();
	
	int length = order.size();
	
	for(int i = 0; i < length; ++i)
	{
	  int maxval = order[i].getMax();
	  int minval = order[i].getMin();
	  
	  if((maxval != minval) && ((maxval - minval) < dom_size) )
	  {
		dom_size = maxval - minval;
		smallest_dom = i;
		if( maxval - minval == 1)
		{ // Binary domain, must be smallest
		  return i;
		}
	  }
	}
   return smallest_dom;
  }

  template<typename Variables>
	inline void solve_loop_sdf(Variables& v)
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean SDF Search");
	  vector<int> branches;
	  branches.reserve(1000);
	  
	  maybe_print_search_state("Node: ", v);
	  
	  while(true)
	  {
		nodes++;
		if(nodes == commandlineoption_nodelimit)
		  return;
	
		if(do_checks())
		  return;
		
		int next_var = find_smallest_domain(v);
		if(next_var == -1)
		{  
		  maybe_print_search_state("Sol: ", v);
		  
		  // We have found a solution!
		  check_sol_is_correct();
		  // This function may escape from search if solution limit
		  // has been reached.
		  deal_with_solution();

		  // fail here to force backtracking.
		  failed = true;
		}
		else
		{
		  maybe_print_search_state("Node: ", v);
		  world_push();
		  
		  // branch left
		  branches.push_back(next_var);
		  int assign_val = v[next_var].getMin();
		  v[next_var].uncheckedAssign(assign_val);
		  propogate_queue();
		}
		
		// Either search failed, or a solution was found.
		while(failed)
		{
		  failed = false;
		  
		  
		  maybe_print_search_state("Node: ", v);		  
		  if(branches.empty())
			throw 0;
		  world_pop();
		  next_var = branches.back();
		  branches.pop_back();
		  
		  v[next_var].setMin(v[next_var].getMin() + 1);
		  set_optimise_and_propogate_queue();
		}
	  }
  }
  

  template<typename VarArray>
  inline void solve_sdf(VarArray& vars)
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
	  solve_loop_sdf(vars);
  }
}




