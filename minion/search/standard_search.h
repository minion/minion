/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#include "common_search.h"

namespace Controller
{

#include "VariableOrders.h"
  
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)

  template<typename VarOrder, typename Variables>
	inline void solve_loop(VarOrder& order, Variables& v)
  {
	  D_INFO(0, DI_SOLVER, "Non-Boolean Search");
	  
	  maybe_print_search_state("Node: ", v);
	  
	  while(true)
	  {
		state->incrementNodeCount();
		if(state->getNodeCount() == options->nodelimit)
		  return;
	
		if(do_checks())
		  return;
		
		// order.find_next_unassigned returns true if all variables assigned.
		if(order.find_next_unassigned())
		{  		  
		  // We have found a solution!
		  check_sol_is_correct();
		  // This function may escape from search if solution limit
		  // has been reached.
		  deal_with_solution();

		  // fail here to force backtracking.
		  state->setFailed(true);
		}
		else
		{
		  maybe_print_search_state("Node: ", v);
		  world_push();
		  order.branch_left();
		  propagate_queue();
		}
		
		// Either search failed, or a solution was found.
		while(state->isFailed())
		{
		  state->setFailed(false);
		  
		  if(order.finished_search())
			return;

		  world_pop();
          maybe_print_search_action("bt");
		  order.branch_right();
		  set_optimise_and_propagate_queue();
		}
	  }
  }
  
}




