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

/** @help constraints;nvalueleq Description
The constraint
 
   pow(x,y,z)

ensures that x^y=z.
*/


#ifndef CONSTRAINT_NVALUE_H
#define CONSTRAINT_NVALUE_H

#include <math.h>
#include "../constraints/constraint_checkassign.h"


template<typename VarArray, typename VarResult>
struct LessEqualNvalueConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "nvalueleq"; }
  
  VarArray vars;
  VarResult result;

  CONSTRAINT_ARG_LIST2(vars, result);
  
  LessEqualNvalueConstraint(StateObj* _stateObj, VarArray _vars, VarResult _result) :
    AbstractConstraint(_stateObj), vars(_vars),result(_result)
  {
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      t.push_back(make_trigger(vars[i], Trigger(this, i), Assigned));
    }
    
    t.push_back(make_trigger(result, Trigger(this, -1), LowerBound));
    return t;
  }
  
  virtual void propagate(DomainInt flag, DomainDelta)
  {
    full_propagate();
  }


  virtual void full_propagate()
  { 
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      if(vars[i].isAssigned())
      {
        assigned.insert(vars[i].getAssignedValue());
      }
    }

    result.setMin(assigned.size());
    
    if(result.getMax() == (DomainInt)assigned.size() && assigned.size() > 0)
    {
      for(unsigned i = 0; i < vars.size(); ++i)
      {
        vars[i].setMin(*assigned.begin());
        vars[i].setMax(*(--assigned.end()));
      }
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
      assigned.insert(v[i]);

    return (DomainInt)assigned.size() <= v[vars.size()];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < vars.size(); ++i)
      v.push_back(vars[i]);
    v.push_back(result);
    return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {  
     for(unsigned i = 0; i < vars.size(); ++i)
     {
       if(vars[i].getMin() != vars[i].getMax())
       {
         assignment.push_back(make_pair(i, vars[i].getMin()));
         assignment.push_back(make_pair(i, vars[i].getMax()));
         return true;
       }
     }

     if(result.getMin() != result.getMax())
     {
       assignment.push_back(make_pair(vars.size(), result.getMin()));
       assignment.push_back(make_pair(vars.size(), result.getMax()));
       return true;
     }

     std::set<DomainInt> values;
     for(unsigned i = 0; i < vars.size(); ++i)
       values.insert(vars[i].getAssignedValue());

     return (DomainInt)values.size() <= result.getAssignedValue();
   }
    
     // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint()
  {
      return forward_check_negation(stateObj, this);
  }
};


template<typename VarArray, typename VarResult>
struct GreaterEqualNvalueConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "nvaluegeq"; }
  
  VarArray vars;
  VarResult result;

  CONSTRAINT_ARG_LIST2(vars, result);
  
  GreaterEqualNvalueConstraint(StateObj* _stateObj, VarArray _vars, VarResult _result) :
    AbstractConstraint(_stateObj), vars(_vars),result(_result)
  {
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      t.push_back(make_trigger(vars[i], Trigger(this, i), Assigned));
    }
    
    t.push_back(make_trigger(result, Trigger(this, -1), LowerBound));
    return t;
  }
  
  virtual void propagate(DomainInt flag, DomainDelta)
  {
    full_propagate();
  }


  virtual void full_propagate()
  { 
    std::set<DomainInt> assigned;
    DomainInt min_unassigned = INT_MAX;
    DomainInt max_unassigned = INT_MIN;
    DomainInt unassigned_count = 0;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      if(vars[i].isAssigned())
      {
        assigned.insert(vars[i].getAssignedValue());
      }
      else
      {
        unassigned_count++;
        min_unassigned = std::min(min_unassigned, vars[i].getMin());
        max_unassigned = std::max(max_unassigned, vars[i].getMax());
      }
    }

    if(unassigned_count == 0)
    {
      result.setMax(assigned.size());
      return;
    }
     
    DomainInt unassigned_estimate = std::min(unassigned_count, max_unassigned - min_unassigned + 1);

    result.setMax(assigned.size() + unassigned_estimate);
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
      assigned.insert(v[i]);

    return (DomainInt)assigned.size() >= v[vars.size()];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < vars.size(); ++i)
      v.push_back(vars[i]);
    v.push_back(result);
    return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {  
     for(unsigned i = 0; i < vars.size(); ++i)
     {
       if(vars[i].getMin() != vars[i].getMax())
       {
         assignment.push_back(make_pair(i, vars[i].getMin()));
         assignment.push_back(make_pair(i, vars[i].getMax()));
         return true;
       }
     }

     if(result.getMin() != result.getMax())
     {
       assignment.push_back(make_pair(vars.size(), result.getMin()));
       assignment.push_back(make_pair(vars.size(), result.getMax()));
       return true;
     }

     std::set<DomainInt> values;
     for(unsigned i = 0; i < vars.size(); ++i)
       values.insert(vars[i].getAssignedValue());

     return (DomainInt)values.size() >= result.getAssignedValue();
   }
    
     // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint()
  {
      return forward_check_negation(stateObj, this);
  }
};

#endif
