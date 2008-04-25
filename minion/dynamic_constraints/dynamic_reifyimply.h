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

#ifndef DYNAMIC_REIFY_TRUE_H
#define DYNAMIC_REIFY_TRUE_H

#include "../constraints/constraint_dynamic.h"
#include "../reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"


template<typename BoolVar>
struct Dynamic_reify_true : public DynamicConstraint
{
  virtual string constraint_name()
  { return "DynamicReifyTrue:" + poscon->constraint_name(); }
  
  DynamicConstraint* poscon;
  BoolVar rar_var;
  bool constraint_locked;
  
  Reversible<bool> full_propagate_called;
  
  Dynamic_reify_true(StateObj* _stateObj, DynamicConstraint* _poscon, BoolVar v) : DynamicConstraint(_stateObj), poscon(_poscon), 
                                                                           rar_var(v), constraint_locked(false),
                                                                           full_propagate_called(stateObj, false)
  { }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    DomainInt back_val = *(v + v_size - 1);
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
  
  virtual int dynamic_trigger_count() 
  { return 1; }
  
  virtual void setup()
  {
    DynamicConstraint::setup();
    
    poscon->setup();
    DynamicTrigger* start = poscon->dynamic_trigger_start();
    int trigs = poscon->dynamic_trigger_count();
    
    for(int i = 0; i < trigs; ++i)
      (start + i)->constraint = this;
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
  
  PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
	  PROP_INFO_ADDONE(ReifyTrue);
    D_INFO(1,DI_REIFY,"Propagation Start");
    if(constraint_locked)
	    return;
	  
  	if(trig == dynamic_trigger_start())
    {
      D_INFO(1,DI_REIFY,"Full Pos Propagation");
	    constraint_locked = true;
	    getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }
    
    if(full_propagate_called)
    {
      D_ASSERT(rar_var.isAssigned() && rar_var.getAssignedValue() == 1);
      poscon->propagate(trig);
    }
    else
    {
      // This is an optimisation.
      trig->remove();
    }
  }
  
  virtual void full_propagate()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    
    rar_var.addDynamicTrigger(dt, LowerBound);
    
    if(rar_var.isAssigned() && rar_var.getAssignedValue() > 0)
    {
	    poscon->full_propagate();
	    full_propagate_called = true;
    }
  }
};


// Just a placeholder.
template<typename BoolVar>
DynamicConstraint*
truereifyCon(StateObj* stateObj, DynamicConstraint* c, BoolVar var)
{ return new Dynamic_reify_true<BoolVar>(stateObj, &*c, var); }

#endif
