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

struct triple {
  bool isLeft;
  unsigned var;
  DomainInt val;

  triple(bool _isLeft, unsigned _var, DomainInt _val) : isLeft(_isLeft), var(_var), val(_val) {}
  friend std::ostream& operator<<(std::ostream& o, const triple& t)
  { o << "(" << t.isLeft << "," << t.var << "," << t.val << ")"; return o; }
};

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

template<typename VarType = AnyVarRef, typename BranchType = StaticBranch>
struct VariableOrder
{
  StateObj* stateObj;
  vector<VarType> var_order;
  vector<int> val_order;
  vector<triple> branches; //L & R branches so far (isLeftBranch?,var,value)
  vector<int> first_unassigned_variable;
  unsigned pos;
  unsigned depth; //number of left branches
  
  BranchType branch_method;
  
  
  VariableOrder(StateObj* _stateObj, vector<VarType>& _varorder, vector<int>& _valorder)
  : stateObj(_stateObj), var_order(_varorder), val_order(_valorder)
  {
    // if this isn't enough room, the vector will autoresize. While that can be slow,
    // it only has to happen at most the log of the maximum search depth.
    branches.reserve(var_order.size());
    first_unassigned_variable.reserve(var_order.size());
    pos = 0; 
    depth = 0;
  }
  
  void reset()
  {
    branches.clear();
    first_unassigned_variable.clear();
    pos = 0;
    depth = 0;
  }
      
  // Returns true if all variables assigned
  bool find_next_unassigned()
  {
    pos = branch_method(var_order, pos);
    return pos == var_order.size();
  }
  
  bool finished_search()
  { return depth == 0; }
  
  int search_depth()
  { return depth; }
  
  void branch_left()
  {
    D_ASSERT(!var_order[pos].isAssigned()) 
    DomainInt assign_val;
    if(val_order[pos])
      assign_val = var_order[pos].getMin();
    else
      assign_val = var_order[pos].getMax();
    var_order[pos].decisionAssign(assign_val);
    maybe_print_search_assignment(stateObj, var_order[pos], assign_val, true);
    branches.push_back(triple(true, pos, assign_val));
    depth++;
    first_unassigned_variable.push_back(pos);
  }
  
  int get_current_pos()
  { return pos; }
  
  void force_branch_left(int new_pos)
  {
    D_ASSERT(new_pos >= 0 && new_pos < var_order.size()); 
    D_ASSERT(!var_order[new_pos].isAssigned()) 
    DomainInt assign_val;
    if(val_order[new_pos])
      assign_val = var_order[new_pos].getMin();
    else
      assign_val = var_order[new_pos].getMax();
    var_order[new_pos].uncheckedAssign(assign_val);
    maybe_print_search_assignment(stateObj, var_order[new_pos], assign_val, true, true);
    branches.push_back(triple(true, new_pos, assign_val));
    depth++;
    // The first unassigned variable could still be much earlier.
    first_unassigned_variable.push_back(pos);
  }
  
  void branch_right()
  {  
     while(!branches.back().isLeft) { //pop off all the RBs
       branches.pop_back();
     }
     int other_branch = branches.back().var; //then the LB is the next to branch on
     branches.pop_back();
     depth--;

     if(val_order[other_branch])
     {
       D_ASSERT(var_order[other_branch].getMax() >= var_order[other_branch].getMin() + 1);
       const DomainInt var_min = var_order[other_branch].getMin();
       maybe_print_search_assignment(stateObj, var_order[other_branch], var_min, false);
       var_order[other_branch].setMin(var_min + 1);
       branches.push_back(triple(false, other_branch, var_min));
     }
     else
     {
       D_ASSERT(var_order[other_branch].getMax() >= var_order[other_branch].getMin() + 1);
       const DomainInt var_max = var_order[other_branch].getMax();
       maybe_print_search_assignment(stateObj, var_order[other_branch], var_max, false);
       var_order[other_branch].setMax(var_max - 1);
       branches.push_back(triple(false, other_branch, var_max));
     }

    pos = first_unassigned_variable.back();
    first_unassigned_variable.pop_back();
  }
};

#endif
