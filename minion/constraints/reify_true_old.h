/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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

#ifndef REIFY_TRUE_OLD_H
#define REIFY_TRUE_OLD_H

#include "constraint_abstract.h"
#include "../minion.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"
#include "../dynamic_constraints/old_dynamic_reifyimply.h"


template<typename BoolVar>
struct reify_true_old : public AbstractConstraint
{
  virtual string constraint_name()
  { return "ReifyTrue:" + poscon->constraint_name(); }
  
  AbstractConstraint* poscon;
  BoolVar rar_var;
  bool constraint_locked;
  
  Reversible<bool> full_propagate_called;
  
  reify_true_old(StateObj* _stateObj, AbstractConstraint* _poscon, BoolVar v) : AbstractConstraint(_stateObj), poscon(_poscon), 
                                                                            rar_var(v), constraint_locked(false),
                                                                            full_propagate_called(stateObj, false)
  { }
  
  virtual AbstractConstraint* reverse_constraint()
  { D_FATAL_ERROR("You can't reverse a reified Constraint!"); }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    
    DomainInt back_val = *(v + v_size - 1);
    //v.pop_back();
    if(back_val != 0)
      return poscon->check_assignment(v, v_size - 1);
    else
      return true;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
	vector<AnyVarRef> vec = poscon->get_vars();
	vec.push_back(rar_var);
	return vec;
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection postrig = poscon->setup_internal();
    triggerCollection triggers;
    for(unsigned int i=0;i<postrig.size();i++)
    {
      postrig[i]->trigger.constraint = this;
      D_ASSERT(postrig[i]->trigger.info != -99999);
      triggers.push_back(postrig[i]);
    }
    triggers.push_back(make_trigger(rar_var, Trigger(this, -99999), LowerBound));
    return triggers;
  }
  
  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    poscon->full_propagate();
    full_propagate_called = true;
  }
  
  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(ReifyTrue);
    if(constraint_locked)
      return;

    if(i == -99999)
    {
      constraint_locked = true;
      getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }
    
    if(full_propagate_called)
    {
      D_ASSERT(rar_var.isAssigned() && rar_var.getAssignedValue() == 1);
      poscon->propagate(i, domain);
    }
    
    #ifdef MINION_DEBUG
    bool flag;
    GET_ASSIGNMENT(assignment0, poscon);
    bool unsat = poscon->check_unsat(i, domain);
    D_ASSERT((!flag && unsat) || (flag && !unsat));
    #endif
    if(poscon->check_unsat(i, domain)) 
    { rar_var.propagateAssign(false); }

  }
  
  virtual void full_propagate()
  {
    #ifdef MINION_DEBUG
    {
      bool flag;
      GET_ASSIGNMENT(assignment0, poscon);
      bool unsat = poscon->full_check_unsat();
      D_ASSERT((!flag && unsat) || (flag && !unsat));
    }
    #endif
    
    if(poscon->full_check_unsat())
      rar_var.propagateAssign(false);

    if(rar_var.isAssigned() && rar_var.getAssignedValue() > 0)
    {
      poscon->full_propagate();
      full_propagate_called = true;
    }
  }
};

// From dynamic_reifyimply.h
template<typename BoolVar>
AbstractConstraint*
truereifyConDynamicOld(StateObj* stateObj, AbstractConstraint* c, BoolVar var);

template<typename BoolVar>
AbstractConstraint*
truereifyConOld(StateObj* stateObj, AbstractConstraint* c, BoolVar var)
{ 
  if(c->dynamic_trigger_count() == 0)
    return new reify_true_old<BoolVar>(stateObj, &*c, var); 
  else
    return truereifyConDynamicOld(stateObj, c, var);
}

template<typename VarArray>
inline AbstractConstraint*
BuildCT_REIFYIMPLY_OLD(StateObj* stateObj, const VarArray& vars, ConstraintBlob& bl)
{
  D_ASSERT(bl.internal_constraints.size() == 1);
  D_ASSERT(vars.size() == 1);
  return truereifyConOld(stateObj, build_constraint(stateObj, bl.internal_constraints[0]), vars[0]);
}

#endif
