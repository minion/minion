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
    if(do_checks(stateObj, order))
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




