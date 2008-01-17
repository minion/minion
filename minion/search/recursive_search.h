/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt 

$Id: standard_search.h 478 2006-11-24 09:42:10Z azumanga $
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
  inline void solve_loop_recursive(VarOrder& order, Variables& v)
  {
	
	maybe_print_search_state("Node: ", v);
	
	nodes++;
	if(nodes == commandlineoption_nodelimit)
	  return;
	if(do_checks())
		return;
	
	// order.find_next_unassigned returns true if all variables assigned.
	if(order.find_next_unassigned())
	{  	  
	  // We have found a solution!
	  check_sol_is_correct();
	  deal_with_solution();
	  
	  // fail here to force backtracking.
	  return;
	}
	
	maybe_print_search_state("Node: ", v);
	world_push();
	order.branch_left();
	propagate_queue();
	if(!failed)
	  solve_loop_recursive(order, v);
	
	failed = false;
	
	world_pop();
	order.branch_right();
    set_optimise_and_propagate_queue();
	
	if(!failed)
	  solve_loop_recursive(order, v);
}
}




