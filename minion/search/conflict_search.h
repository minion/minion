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

namespace Controller
{
  /// Variable Order objects
  // These need the following functions:
  // Constructor that takes existing variable and value ordering
  // (Feel free to ignore the value ordering!)
  
  // This function implements the algorithm in the ECAI '06 paper,
  // Last Conﬂict based Reasoning 
  template<typename VarOrder, typename Variables, typename Propogator>
  inline void conflict_solve_loop(StateObj* stateObj, function<void (void)> next_search, VarOrder& order, Variables& v, Propogator prop = PropagateGAC())
  {
    maybe_print_search_state(stateObj, "Node: ", v);
    int last_conflict_var = -1;
    while(true)
    {
      getState(stateObj).incrementNodeCount();
      if(do_checks(stateObj, order))
        return;
      
      D_ASSERT(last_conflict_var >= -1 && last_conflict_var < (int)v.size());
      // Clear the 'last conflict var if it has got assigned'
      if(last_conflict_var != -1 && v[last_conflict_var].isAssigned())
        last_conflict_var = -1;
      
      // order.find_next_unassigned returns true if all variables assigned.
      if(order.find_next_unassigned())
      {
        // This function may escape from search if solution limit
        // has been reached.
        next_search();
        // fail here to force backtracking.
        getState(stateObj).setFailed(true);
      }
      else
      {
        maybe_print_search_state(stateObj, "Node: ", v);
        world_push(stateObj);
        if(last_conflict_var == -1)
          order.branch_left();
        else
          order.force_branch_left(last_conflict_var);
        prop(stateObj, v);
      }
      
      if(!getState(stateObj).isFailed())
      {
        // Last conflict var is satisfied
        last_conflict_var = -1;  
      }
      else
      {
        // Either search failed, or a solution was found.
        while(getState(stateObj).isFailed())
        {
          // the order.get_current_pos() != v.size() is in case we just got a soln.
          if(last_conflict_var == -1 && order.get_current_pos() != v.size())
            last_conflict_var = order.get_current_pos();
          
          getState(stateObj).setFailed(false);
          
          if(order.finished_search())
            return;
          
          world_pop(stateObj);
          maybe_print_search_action(stateObj, "bt");

          order.branch_right();

          set_optimise_and_propagate_queue(stateObj, prop, v);
        }
      }
    }
  }
  
}
