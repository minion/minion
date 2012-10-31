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

// This is a standard fall-back constraint which can be used when a
// constraint has not got a better method of implementing their
// inverse for reification.

#ifndef CONSTRAINT_CHECKASSIGN_H
#define CONSTRAINT_CHECKASSIGN_H

template<typename VarArray, typename OriginalConstraint>
struct CheckAssignConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "!!CheckAssign"; }
  
  OriginalConstraint& originalcon;
  VarArray variables;
  ReversibleInt assigned_vars;
  
  CheckAssignConstraint(StateObj* _stateObj, const VarArray& vars, OriginalConstraint& con)
  : AbstractConstraint(_stateObj), originalcon(con),variables(vars), assigned_vars(stateObj)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      t.push_back(make_trigger(variables[i], Trigger(this, i), Assigned));
    return t;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  { 
    cerr << "Check assign constraints shouldn't get reversed." << endl;
    FAIL_EXIT();
    return NULL;
  }
  
  virtual void propagate(DomainInt prop_val,DomainDelta delta)
  {
    PROP_INFO_ADDONE(CheckAssign);
    if(check_unsat(checked_cast<SysInt>(prop_val), delta))
      getState(stateObj).setFailed(true);
  }
  
  virtual BOOL check_unsat(SysInt,DomainDelta)
  {
    MAKE_STACK_BOX(assignment, DomainInt, variables.size());
    SysInt count = assigned_vars;
    ++count;
    SysInt v_size = variables.size();
    D_ASSERT(count <= v_size);

    if(count == v_size)
    {
      for(SysInt i = 0; i < v_size; ++i)
      {
        D_ASSERT(variables[i].isAssigned());
        assignment.push_back(variables[i].getAssignedValue());
      }
      if(assignment.size() == 0)
          return !check_assignment(NULL, 0);
      else
          return !check_assignment(&assignment.front(), assignment.size());
    }
    assigned_vars = count; 
    return false;
  }
  
  virtual BOOL full_check_unsat()
  {
    MAKE_STACK_BOX(assignment, DomainInt, variables.size());
    UnsignedSysInt counter = 0;
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      if(variables[i].isAssigned()) ++counter;
    assigned_vars = counter;
    
    if(counter == variables.size())
    {
      for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      {
        D_ASSERT(variables[i].isAssigned());
        assignment.push_back(variables[i].getAssignedValue());
      }
      if(assignment.size() == 0)
          return !check_assignment(NULL, 0);
      else
          return !check_assignment(&assignment.front(), assignment.size());
    }
    return false;
  }
  
  virtual void full_propagate()
  {
    if(full_check_unsat())
      getState(stateObj).setFailed(true);
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == variables.size());
    return !originalcon.check_assignment(v, v_size);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(variables.size());
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      vars.push_back(variables[i]);
    return vars;
  }
  
  // Getting a satisfying assignment here is too hard.
  // Let's at least try forward checking!
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& ret_box)
  {
    MAKE_STACK_BOX(c, DomainInt, variables.size());

    SysInt free_var = -1;
    for(SysInt i = 0; i < variables.size(); ++i)
    {
      if(!variables[i].isAssigned()) 
      {
        if(free_var != -1)
        {
          ret_box.push_back(make_pair(i, variables[i].getMin()));
          ret_box.push_back(make_pair(i, variables[i].getMax()));
          return true;
        }
      else
        {
          free_var = i;
          c.push_back(0);
        }
      }
      else
        c.push_back(variables[i].getAssignedValue());
    }

    if(free_var == -1)
    {
      if(try_assignment(ret_box, c))
        return true;
    }
    else
    {
      DomainInt free_min = variables[free_var].getMin();
      c[free_var]=free_min;
      if(try_assignment(ret_box, c))
        return true;
      DomainInt free_max = variables[free_var].getMax();
      c[free_var]=free_max;
      if(try_assignment(ret_box, c))
        return false;

      if(!variables[free_var].isBound())
      {
        for(DomainInt i = variables[free_var].getMin() + 1; i < variables[free_var].getMax(); ++i)
        {
          if(variables[free_var].inDomain(i))
          {
            c[free_var] = i;
            if(try_assignment(ret_box, c))
              return true;
          }
        }
      }
      else
      {
          ret_box.push_back(make_pair(free_var, free_min));
          ret_box.push_back(make_pair(free_var, free_max));
          return true;
      }
    }
    return false;
  }

  template<typename Box, typename Check>
  bool try_assignment(Box& assign, Check& check)
  {
    if(check_assignment(check.begin(), check.size()))
    {
      // Put the complete assignment in the box.
      for(SysInt i = 0; i < check.size(); ++i)
        assign.push_back(make_pair(i, check[i]));
      return true;
    }
    else
      return false;
  }
};

#endif
