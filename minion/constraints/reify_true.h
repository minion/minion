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

#ifndef REIFY_TRUE_H
#define REIFY_TRUE_H

#include "constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

template<typename BoolVar, bool DoWatchAssignment>
  struct reify_true : public ParentConstraint
{
  virtual string constraint_name()
    { return "ReifyTrue:" + child_constraints[0]->constraint_name(); }

  BoolVar rar_var;
  bool constraint_locked;

  Reversible<bool> full_propagate_called;

  reify_true(StateObj* _stateObj, AbstractConstraint* _poscon, BoolVar _rar_var) :
  ParentConstraint(_stateObj), rar_var(_rar_var), constraint_locked(false),
    full_propagate_called(stateObj, false)
  {
    child_constraints.push_back(_poscon);
  }

  virtual AbstractConstraint* reverse_constraint()
  { D_FATAL_ERROR("You can't reverse a reified Constraint!"); }

  virtual int dynamic_trigger_count()
  {
    if(DoWatchAssignment)
      return child_constraints[0]->get_vars_singleton()->size()*2;
    else
      return 0;
  }

  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(rar_var.inDomain(0))
    {
      D_ASSERT(get_vars()[child_constraints[0]->get_vars_singleton()->size()].inDomain(0));
      assignment.push_back(make_pair(child_constraints[0]->get_vars_singleton()->size(), 0));
      return true;
    }
    else if(rar_var.inDomain(1))
    { 

      bool ret = child_constraints[0]->get_satisfying_assignment(assignment);
      if(ret)
      {
        assignment.push_back(make_pair(child_constraints[0]->get_vars_singleton()->size(), 1));
        return true;
      }
      else
        return false;
    }
    else
      return false;
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    DomainInt back_val = *(v + v_size - 1);
    //v.pop_back();
    if(back_val == 1)
      return child_constraints[0]->check_assignment(v, v_size - 1);
    else return (back_val == 0);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vec = child_constraints[0]->get_vars();
    vec.push_back(rar_var);
    return vec;
  }

  virtual triggerCollection setup_internal()
  {
    triggerCollection triggers;
    triggers.push_back(make_trigger(rar_var, Trigger(this, -1), LowerBound));
    return triggers;
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    P("Special Check!");
    constraint_locked = false;
    child_constraints[0]->full_propagate();
    full_propagate_called = true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    P("Special unlock!");
    constraint_locked = false;
  }

  virtual void propagate(int i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(ReifyTrue);
    P("Static propagate start");
    if(constraint_locked)
      return;

    if(i == -1)
    {
        if(!rar_var.isAssigned() || rar_var.getAssignedValue()==0 )
            return;
        D_ASSERT(rar_var.getAssignedValue() == 1);
        P("rarvar assigned to 1- Do full propagate");
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
        return;
    }

    if(full_propagate_called)
    {
      P("Already doing static full propagate");
      D_ASSERT(rar_var.isAssigned() && rar_var.getAssignedValue() == 1);
      pair<int,int> childTrigger = getChildStaticTrigger(i);
      D_ASSERT(childTrigger.first == 0);
      P("Passing trigger" << childTrigger.second << "on");
      child_constraints[0]->propagate(childTrigger.second, domain);
    }
  }

  virtual void propagate(DynamicTrigger* trig)
  {
    PROP_INFO_ADDONE(ReifyTrue);
    P("Dynamic prop start");
    if(constraint_locked)
      return;

    DynamicTrigger* dt = dynamic_trigger_start();

    if(DoWatchAssignment && trig >= dt && trig < dt + dynamic_trigger_count())
    {// Lost assignment, but don't replace when rar_var=0
      P("Triggered on an assignment watch");
      if(!full_propagate_called && !rar_var.isAssigned())
      {
        bool flag;
        GET_ASSIGNMENT(assignment, child_constraints[0]);
        PROP_INFO_ADDONE(ReifyImplyGetSatAssg);

        P("Find new assignment");
        if(!flag)
        { // No satisfying assignment to constraint
          P("Failed!");
          rar_var.propagateAssign(0);
          return;
        }
        P("Found new assignment");
        watch_assignment(assignment, *(child_constraints[0]->get_vars_singleton()), dt);
      }
      return;
    }

    if(full_propagate_called)
    {
      P("Pass triggers to children");
      D_ASSERT(rar_var.isAssigned() && rar_var.getAssignedValue() == 1);
      child_constraints[0]->propagate(trig);
    }
    else
    {
      P("Remove unused trigger");
      // This is an optimisation.
      releaseTrigger(stateObj, trig);
    }
  }

  template<typename T, typename Vars, typename Trigger>
  void watch_assignment(const T& assignment, Vars& vars, Trigger* trig)
  {
    for(int i = 0; i < assignment.size(); ++i)
    {
      D_ASSERT(vars[assignment[i].first].inDomain(assignment[i].second));
      if(vars[assignment[i].first].isBound()) {
        vars[assignment[i].first].addDynamicTrigger(trig + i, DomainChanged);
      } else {
        vars[assignment[i].first].addDynamicTrigger(trig + i, DomainRemoval, assignment[i].second);
      }
    }

  }

  virtual void full_propagate()
  {
    P("Full prop");    
    P(child_constraints[0]->constraint_name());
    rar_var.setMin(0);
    rar_var.setMax(1);
    if(rar_var.isAssigned() && rar_var.getAssignedValue() == 1)
    {
      child_constraints[0]->full_propagate();
      full_propagate_called = true;
      return;
    }
    
    DynamicTrigger* dt = dynamic_trigger_start();
    int dt_count = dynamic_trigger_count();
    // Clean up triggers
    for(int i = 0; i < dt_count; ++i)
        releaseTrigger(stateObj, dt);
    
    if(DoWatchAssignment && !rar_var.isAssigned()) //don't place when rar_var=0
    {
        bool flag;
        GET_ASSIGNMENT(assignment, child_constraints[0]);
        PROP_INFO_ADDONE(ReifyImplyGetSatAssg);
        if(!flag)
        {   // No satisfying assignment to constraint
            P("Assigning reifyvar to 0");
            rar_var.propagateAssign(0);
            return;
        }
        watch_assignment(assignment, *(child_constraints[0]->get_vars_singleton()), dt);
    }
  }
};

template<typename BoolVar>
AbstractConstraint*
truereifyCon(StateObj* stateObj, AbstractConstraint* c, BoolVar var)
{ return new reify_true<BoolVar, true>(stateObj, &*c, var); }

template<typename BoolVar>
AbstractConstraint*
truereifyQuickCon(StateObj* stateObj, AbstractConstraint* c, BoolVar var)
{ return new reify_true<BoolVar, false>(stateObj, &*c, var); }
#endif
