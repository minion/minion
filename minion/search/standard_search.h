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

#ifndef STANDARD_SEARCH_H
#define STANDARD_SEARCH_H

#include "common_search.h"
#include "../system/system.h"

namespace Controller
{

#include "VariableOrders.h"

  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)
  
  template<typename VarOrder, typename Variables, typename Propogator>
  inline void solve_loop(StateObj* stateObj, function<void (void)> next_search, VarOrder& order, Variables& v, Propogator prop, bool findOneSol)
  {
    // Don't corrupt the state the world came in with.
    world_push(stateObj);
    
    maybe_print_search_state(stateObj, "Node: ", v);
    D_ASSERT(getQueue(stateObj).isQueuesEmpty());
    while(true)
    {
      D_ASSERT(getQueue(stateObj).isQueuesEmpty());
      getState(stateObj).incrementNodeCount();
      if(do_checks(stateObj, order))
        throw EndOfSearch();

      // order.find_next_unassigned returns true if all variables assigned.
      if(order.find_next_unassigned())
      {
        // This function may escape from search if solution limit
        // has been reached.
        next_search();
        
        // This is not for overall solution counting, but auxillary variables.
        if(findOneSol) 
        {
          int search_depth = order.search_depth();
          for(int i = 0; i < search_depth; ++i)
            world_pop(stateObj);
          // One more to remove the initial save.
          order.reset();
          world_pop(stateObj);
          cout << "Solution Leaving" << endl;
          return;
        }
        
        // fail here to force backtracking.
        getState(stateObj).setFailed(true);
      }
      else
      {
        maybe_print_search_state(stateObj, "Node: ", v);
        world_push(stateObj);
        order.branch_left();
        prop(stateObj, v);
        D_ASSERT(getQueue(stateObj).isQueuesEmpty() || getState(stateObj).isFailed());
      }

      // Either search failed, or a solution was found.
      while(getState(stateObj).isFailed())
      {
        getState(stateObj).setFailed(false);
        if(order.finished_search())
        {
          order.reset();
          world_pop(stateObj);
          cout << "Search end leaving" << endl;
          return;
        }
        world_pop(stateObj);
        maybe_print_search_action(stateObj, "bt");
        order.branch_right();
        set_optimise_and_propagate_queue(stateObj);
      }
    }
  }

}

#endif
