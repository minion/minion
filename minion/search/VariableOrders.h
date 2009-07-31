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

#ifndef VARIABLEORDERS_H
#define VARIABLEORDERS_H

#include "search_methods.h"

template<typename T>
void inline maybe_print_search_assignment(StateObj* stateObj, T& var, DomainInt val, BOOL equal, bool force = false)
{
    if(getOptions(stateObj).dumptree)
    {
      if(force)
        cout << "ForceAssign: " << var << (equal?" = ":" != ") << val << endl;
      else
        cout << "SearchAssign:" << var << (equal?" = ":" != ") << val << endl;
    }
}

// instead of carrying around the pos everywhere, the VariableOrder object has pos in it as a reversible<int>. 

// replace find_next_unassigned with all_vars_assigned (which maybe uses something like a watch).

// Need a ceiling which comes down as work is stolen, and a steal_work function.

// remove template on branchtype. make virtual.

template<typename Propagator, typename VarType = AnyVarRef>   // for the time being vartype  must be anyvarref.
struct SearchManager 
{
  StateObj* stateObj;
  vector<VarType> var_array;
  VariableOrder var_order;
  
  vector<triple> branches; //L & R branches so far (isLeftBranch?,var,value)
  //vector<int> first_unassigned_variable;
  
  unsigned depth; //number of left branches
  unsigned ceiling; // index into branches, it is the lowest LB which has been stolen.
    
  SearchManager(StateObj* _stateObj, vector<VarType> _var_array, VariableOrder _var_order)
  : stateObj(_stateObj), var_order(_var_order), var_array(_var_array), depth(0), ceiling(-1)
  {
    // if this isn't enough room, the vector will autoresize. While that can be slow,
    // it only has to happen at most the log of the maximum search depth.
    branches.reserve(var_order.size());
  }
  
  void reset()
  {
    branches.clear();
    depth = 0;
  }
  
  // Returns true if all variables assigned
  bool all_vars_assigned()
  {
    pair<int, DomainInt> picked = var_order.pickVarVal();
    return picked.first == -1;
  }
    
    // this is weird: what if we just started search, or only have right-branches above?
    bool finished_search()
    { return depth == 0; }
    
    int search_depth()
    { return depth; }
    
    // returns false if left branch not possible.
    bool branch_left()
    {
        pair<int, DomainInt> picked = var_order.pickVarVal();
        if(picked.first == -1)
        {
            return false;
        }
        D_ASSERT(!var_order[picked.first].isAssigned())
        var_order[picked.first].decisionAssign(picked.second);
        maybe_print_search_assignment(stateObj, var_order[pos], assign_val, true);
        branches.push_back(triple(true, picked.first, picked.second));
        depth++;
        return true;
    }
    
    // Only for conflict search?
    void force_branch_left(int new_pos)
    {
        D_ASSERT(new_pos >= 0 && new_pos < var_order.size()); 
        D_ASSERT(!var_order[new_pos].isAssigned()) 
        
        DomainInt assign_val;
        //if(val_order[new_pos])
        assign_val = var_order[new_pos].getMin();
        //else
        //assign_val = var_order[new_pos].getMax();
        
        var_order[new_pos].uncheckedAssign(assign_val);
        maybe_print_search_assignment(stateObj, var_order[new_pos], assign_val, true, true);
        branches.push_back(triple(true, new_pos, assign_val));
        depth++;
    }
    
    bool branch_right()
    {
        while(!branches.empty() && !branches.back().isLeft) { //pop off all the RBs
            branches.pop_back();
        }
        if((branches.size()-1)<=ceiling)
        {   // if idx of last element is less than or equal the ceiling. 
            // Also catches the empty case.
            return false;
        }
        
        int var = branches.back().var
        DomainInt val = branches.back().val
        branches.pop_back();
        depth--;
        D_ASSERT(var_array[var].inDomain(val));
        
        // special case the upper and lower bounds to make it work for bound variables
        if(var_array[var].getMin() == val)
        {
            var_array[var].setMin(val+1);
        }
        else if(var_array[var].getMax() == val)
        {
            var_array[var].setMax(val-1);
        }
        else
        {
            var_array[var].removeFromDomain(val);
        }
        maybe_print_search_assignment(stateObj, var_array[var], val, false);
        branches.push_back(triple(false, other_branch, var_min));
    }
    
    pair<int, DomainInt> steal_work()
    {   // steal the topmost left branch from this search.
        unsigned newceiling=ceiling+1;
        unsigned b_size=branches.size();
        
        while(newceiling<b_size)
        {
            if(branches[newceiling].isLeft)
            {
                ceiling=newceiling;
                break;
            }
            newceiling++;
        }
        if(newceiling==b_size)
            return make_pair(-1, 0);
        else
            return make_pair(branches[ceiling].var, branches[ceiling].val);
    }
    
    // Most basic search procedure
    virtual void search(Propagator prop, )
    {
        maybe_print_search_state(stateObj, "Node: ", v);
        while(true)
        {
            getState(stateObj).incrementNodeCount();
            if(do_checks(stateObj, order))
                return;
            
            pair<int, DomainInt> varval= var_order.pickVarVal();
            
            if(varval.first==-1)
            {
                // fail here to force backtracking.
                getState(stateObj).setFailed(true);
            }
            else
            {
                maybe_print_search_state(stateObj, "Node: ", v);
                world_push(stateObj);
                branch_left();
                prop(stateObj, v);
            }
            
            if(getState(stateObj).isFailed())
            {
                // Either search failed, or a solution was found.
                while(getState(stateObj).isFailed())
                {
                    getState(stateObj).setFailed(false);
                    
                    if(finished_search())
                        return;
                    
                    world_pop(stateObj);
                    maybe_print_search_action(stateObj, "bt");
                    
                    branch_right();
                    
                    set_optimise_and_propagate_queue(stateObj);
                }
            }
        }
    }
};

#endif
