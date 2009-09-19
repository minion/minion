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

#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include "variable_orderings.h"
#include "common_search.h"
#include "../preprocess.h"

namespace Controller
{


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

struct SearchManager 
{
  StateObj* stateObj;
  vector<AnyVarRef> var_array;
  VariableOrder * var_order;
  
  Propagate * prop;  // Propagate is the type of the base class. Method prop.prop(stateObj, var_array)
  
  vector<Controller::triple> branches; //L & R branches so far (isLeftBranch?,var,value)
  //vector<int> first_unassigned_variable;
  
  unsigned depth; //number of left branches
  int ceiling; // index into branches, it is the lowest LB which has been stolen.
    
  SearchManager(StateObj* _stateObj, vector<AnyVarRef> _var_array, VariableOrder* _var_order, Propagate * _prop)
  : stateObj(_stateObj), var_order(_var_order), var_array(_var_array), depth(0), ceiling(-1), prop(_prop)
  {
    // if this isn't enough room, the vector will autoresize. While that can be slow,
    // it only has to happen at most the log of the maximum search depth.
    branches.reserve(var_array.size());
  }
  
  void reset()
  {
    branches.clear();
    depth = 0;
  }
  
  // Returns true if all variables assigned
  bool all_vars_assigned()
  {
    pair<int, DomainInt> picked = var_order->pickVarVal();
    return picked.first == -1;
  }
    
    // this is weird: what if we just started search, or only have right-branches above?
    bool finished_search()
    { return depth == 0; }
    
    int search_depth()
    { return depth; }
    
    // returns false if left branch not possible.
    virtual void branch_left(pair<int, DomainInt> picked)
    {
        D_ASSERT(picked.first!=-1);
        D_ASSERT(!var_array[picked.first].isAssigned());
        
        world_push(stateObj);
        var_array[picked.first].decisionAssign(picked.second);
        maybe_print_search_assignment(stateObj, var_array[picked.first], picked.second, true);
        branches.push_back(Controller::triple(true, picked.first, picked.second));
        depth++;
    }
    
    virtual bool branch_right()
    {
        while(!branches.empty() && !branches.back().isLeft) { //pop off all the RBs
            branches.pop_back();
        }
        
        if((((int)branches.size())-1)<=ceiling)
        {   // if idx of last element is less than or equal the ceiling. 
            // Also catches the empty case.
            return false;
        }
        
        int var = branches.back().var;
        DomainInt val = branches.back().val;
        
        // remove the left branch.
        branches.pop_back();
        world_pop(stateObj);
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
        branches.push_back(Controller::triple(false, var, val));
        return true;
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
    virtual void search()
    {
        maybe_print_search_state(stateObj, "Node: ", var_array);
        while(true)
        {
            D_ASSERT(getQueue(stateObj).isQueuesEmpty());
            
            getState(stateObj).incrementNodeCount();
            
            if(do_checks(stateObj, var_array, branches))
                return;
            
            pair<int, DomainInt> varval= var_order->pickVarVal();
            
            if(varval.first==-1)
            {
                deal_with_solution(stateObj);
                
                // If we are not finished, then go into the loop below.
                getState(stateObj).setFailed(true);
            }
            else
            {
                maybe_print_search_state(stateObj, "Node: ", var_array);
                branch_left(varval);
                prop->prop(stateObj, var_array);
            }
            
            // loop to 
            while(getState(stateObj).isFailed())
            {
                getState(stateObj).setFailed(false);
                if(finished_search())
                {   // what does this do?
                    return;
                }
                
                maybe_print_search_action(stateObj, "bt");
                bool flag=branch_right();
                if(!flag)
                {   // No remaining left branches to branch right.
                    return;
                }
                set_optimise_and_propagate_queue(stateObj);
            }
        }
    }
};

// junk

struct ConflictSearchManager : SearchManager
{
    // Conflict search procedure
    virtual void search()
    {
        maybe_print_search_state(stateObj, "Node: ", var_array);
        int last_conflict_var = -1;
        while(true)
        {
            D_ASSERT(getQueue(stateObj).isQueuesEmpty());
            
            getState(stateObj).incrementNodeCount();
            
            cout << "About to call do_checks" << endl;
            
            if(do_checks(stateObj, var_array, branches))
                return;
            
            D_ASSERT(last_conflict_var >= -1 && last_conflict_var < (int)var_array.size());
            // Clear the 'last conflict var if it has got assigned'
            if(last_conflict_var != -1 && var_array[last_conflict_var].isAssigned())
                last_conflict_var = -1;
            
            cout << "About to call pickVarVal" << endl;
            
            pair<int, DomainInt> varval= var_order->pickVarVal();
            
            cout << varval.first << "," << varval.second <<endl;
            
            if(varval.first==-1)
            {
                deal_with_solution(stateObj);
                
                // If we are not finished, then go into the loop below.
                getState(stateObj).setFailed(true);
            }
            else
            {
                maybe_print_search_state(stateObj, "Node: ", var_array);
                if(last_conflict_var!=-1)
                {
                    //BUG BUG BUG only uses the ascending value order.
                    varval=make_pair(last_conflict_var, var_array[last_conflict_var].getMin());
                }
                branch_left(varval);
                prop->prop(stateObj, var_array);
                
                if(!getState(stateObj).isFailed())
                {
                    last_conflict_var=-1;
                }
            }
            
            // loop to 
            while(getState(stateObj).isFailed())
            {
                // the varval.first != -1 is in case we just got a soln.
                if(last_conflict_var == -1 && varval.first!=-1)
                    last_conflict_var = varval.first;
                
                getState(stateObj).setFailed(false);
                if(finished_search())
                {   // what does this do?
                    return;
                }
                
                maybe_print_search_action(stateObj, "bt");
                bool flag=branch_right();
                if(!flag)
                {   // No remaining left branches to branch right.
                    return;
                }
                set_optimise_and_propagate_queue(stateObj);
            }
        }
    }
};

}

#endif
