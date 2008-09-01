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
  inline void solve_loop_recursive(StateObj* stateObj, VarOrder& order, Variables& v)
  {
    maybe_print_search_state(stateObj, "Node: ", v);

    getState(stateObj).incrementNodeCount();
    if(do_checks(stateObj))
      return;
	
	// order.find_next_unassigned returns true if all variables assigned.
	if(order.find_next_unassigned())
	{  	  
	  // We have found a solution!
	  deal_with_solution(stateObj);
	  
	  // fail here to force backtracking.
	    return;
	}
	
	maybe_print_search_state(stateObj, "Node: ", v);
	world_push(stateObj);
	order.branch_left();
	getQueue(stateObj).propagateQueue();
	if(!getState(stateObj).isFailed())
	  solve_loop_recursive(order, v);
	
	getState(stateObj).setFailed(false);
	
	world_pop(stateObj);
	order.branch_right();
    set_optimise_and_propagate_queue(stateObj);
	
	if(!getState(stateObj).isFailed())
	  solve_loop_recursive(order, v);
}
}




