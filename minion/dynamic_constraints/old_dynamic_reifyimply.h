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

#ifndef DYNAMIC_REIFY_TRUE_OLD_H
#define DYNAMIC_REIFY_TRUE_OLD_H

#include "../constraints/constraint_abstract.h"
#include "../minion.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)


template<typename BoolVar>
struct Dynamic_reify_true_old : public AbstractConstraint
{
  virtual string constraint_name()
  { return "DynamicReifyTrue:" + poscon->constraint_name(); }
  
  AbstractConstraint* poscon;
  BoolVar rar_var;
  bool constraint_locked;
  
  Reversible<bool> full_propagate_called;
  
  Dynamic_reify_true_old(StateObj* _stateObj, AbstractConstraint* _poscon, BoolVar v) : AbstractConstraint(_stateObj), poscon(_poscon), 
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
  { return 1 + poscon->get_vars_singleton()->size(); }
  
  // Override setup!
  virtual void setup()
  {
    AbstractConstraint::setup();
    
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
  
  virtual void propagate(DynamicTrigger* trig)
  {
    PROP_INFO_ADDONE(ReifyTrue);
    P("ReifyImply Prop");
    
    if(constraint_locked)
        return;

    DynamicTrigger* dt = dynamic_trigger_start();
    DynamicTrigger* assign_trigs = dt + 1;
      
    P("Trigger: " << trig - dt);
              
    if(trig == dt)
    {
      D_ASSERT(rar_var.isAssigned() && rar_var.getAssignedValue() == 1);
      constraint_locked = true;
      getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }
    
    if(trig >= assign_trigs && trig < dt + dynamic_trigger_count())
    {// Lost assignment
      if(!full_propagate_called)
      {
        bool flag;
        GET_ASSIGNMENT(assignment, poscon);

        if(!flag)
        { // No satisfying assignment to constraint
          rar_var.propagateAssign(0);
          return;
        }
      
        vector<AnyVarRef>& poscon_vars = *(poscon->get_vars_singleton());

        for(int i = 0; i < assignment.size(); ++i)
        {
          D_ASSERT(poscon_vars[assignment[i].first].inDomain(assignment[i].second));
          if(poscon_vars[assignment[i].first].isBound()) {
            poscon_vars[assignment[i].first].addDynamicTrigger(assign_trigs + i, DomainChanged);
          } else {
            poscon_vars[assignment[i].first].addDynamicTrigger(assign_trigs + i, DomainRemoval, assignment[i].second);
          }
        }
      }
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
      trig->remove(getQueue(stateObj).getNextQueuePtrRef());
    }
  }
  
  virtual void full_propagate()
  {
    P("Reifyimply FullProp");
    DynamicTrigger* dt = dynamic_trigger_start();
    DynamicTrigger* assign_trigs = dt + 1;
    int dt_count = dynamic_trigger_count();
    // Clean up triggers
    for(int i = 0; i < dt_count; ++i)
      dt[i].remove(getQueue(stateObj).getNextQueuePtrRef());
    
    rar_var.addDynamicTrigger(dt, LowerBound);
    bool flag;
    
    GET_ASSIGNMENT(assignment, poscon);
    
    if(!flag)
    { // No satisfying assignment to constraint
      cout << "Found no satisfying assignment!" << endl;
      rar_var.propagateAssign(0);
      return;
    }
    
    vector<AnyVarRef>& poscon_vars = *(poscon->get_vars_singleton());

    for(int i = 0; i < assignment.size(); ++i)
    {
      D_ASSERT(poscon_vars[assignment[i].first].inDomain(assignment[i].second));
      if(poscon_vars[assignment[i].first].isBound()) {
        poscon_vars[assignment[i].first].addDynamicTrigger(assign_trigs + i, DomainChanged);
      } else {
        poscon_vars[assignment[i].first].addDynamicTrigger(assign_trigs + i, DomainRemoval, assignment[i].second);
      }
    }
    if(rar_var.isAssigned() && rar_var.getAssignedValue() > 0)
    {
        poscon->full_propagate();
        full_propagate_called = true;
    }
  }
};



// Just a placeholder.
template<typename BoolVar>
AbstractConstraint*
truereifyConDynamicOld(StateObj* stateObj, AbstractConstraint* c, BoolVar var)
{ return new Dynamic_reify_true_old<BoolVar>(stateObj, &*c, var); }


#endif
