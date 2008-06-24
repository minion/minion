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

/** @help constraints;reify References
See
   help constraints reification
*/

/** @help constraints;reifyimply References
See
   help constraints reification
*/

/** @help constraints;reification Description
Reification is provided in two forms: reify and reifyimply.

   reify(constraint, r) where r is a 0/1 var

ensures that r is set to 1 if and only if constraint is satisfied. That is, if r
is 0 the constraint must NOT be satisfied; and if r is 1 it must be satisfied as
normal. Conversely, if the constraint is satisfied then r must be 1, and if not
then r must be 0.

   reifyimply(constraint, r)

only checks that if r is set to 1 then constraint must be satisfied. If r is not
1, constraint may be either satisfied or unsatisfied. Furthermore r is never set
by propagation, only by search; that is, satisfaction of constraint does not
affect the value of r.
*/

/** @help constraints;reification Notes
Not all constraints are reifiable. Entries for individual constraints give
more information.
*/


// Note: The whole constraint_locked thing is for the following case:
// Consider the following events are on the queue:
// "rareify boolean is assigned, Y is assigned"
// Now "rareify boolean is assigned" causes full_propagate to be called for
// the constraint. It will set up it's data structures based on the current
// assignment. Then later it will be given Y is assigned, but have already
// possibly used that. Confusion follows. Therefore when we want to propagate
// the function, we "lock" it until the queue empties, then start ping
// the constraint.

#ifndef REIFY_H
#define REIFY_H

#include "constraint_abstract.h"
#include "../reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

template<typename BoolVar>
struct reify : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Reify:" + poscon->constraint_name(); }
	
  AbstractConstraint* poscon;
  AbstractConstraint* negcon;
  BoolVar rar_var;
  
  // These two variables are only used in special cases.
  bool constraint_locked;
  Reversible<bool> full_propagate_called;
  
  reify(StateObj* _stateObj, AbstractConstraint* _poscon, BoolVar v) : AbstractConstraint(_stateObj), poscon(_poscon),
                                                               rar_var(v),  constraint_locked(false),
                                                               full_propagate_called(stateObj, false)
  { negcon = poscon->reverse_constraint();}
  
  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "You can't reverse a reified Constraint!";
    FAIL_EXIT();
  }
  
  virtual BOOL check_assignment(DomainInt* vals, int v_size)
  {
    DomainInt back_val = *(vals + (v_size - 1));
    if(back_val != 0)
    {
      return poscon->check_assignment(vals, poscon->get_vars_singleton()->size());
    }
    else
    {
      vals += poscon->get_vars_singleton()->size();
      return negcon->check_assignment(vals, negcon->get_vars_singleton()->size());
    }
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    // We have to push back both sets of variables, as they may
	// have been transformed in different ways.
    vector<AnyVarRef> vec1 = poscon->get_vars();
	vector<AnyVarRef> vec2 = negcon->get_vars();
	vec1.reserve(vec1.size() + vec2.size() + 1);
	for(unsigned i = 0; i < vec2.size(); ++i)
	  vec1.push_back(vec2[i]);
	vec1.push_back(rar_var);
	return vec1;
  }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_REIFY,"Setting up rarification");
    triggerCollection postrig = poscon->setup_internal();
    triggerCollection negtrig = negcon->setup_internal();
    triggerCollection triggers;
    for(unsigned int i=0;i<postrig.size();i++)
    {
      postrig[i]->trigger.info = postrig[i]->trigger.info * 2;
      postrig[i]->trigger.constraint = this;
      triggers.push_back(postrig[i]);
    }
    
    for(unsigned int i=0;i<negtrig.size();i++)
    {
      negtrig[i]->trigger.info = negtrig[i]->trigger.info * 2 + 1;
      negtrig[i]->trigger.constraint = this;
      triggers.push_back(negtrig[i]);
    }
    
    triggers.push_back(make_trigger(rar_var, Trigger(this, -99999), LowerBound));
    triggers.push_back(make_trigger(rar_var, Trigger(this, -99998), UpperBound));
    return triggers;
  }
  
  
  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    D_ASSERT(rar_var.isAssigned());
    if(rar_var.getAssignedValue() > 0)
      poscon->full_propagate();
    else
      negcon->full_propagate();
    full_propagate_called = true;
  }
  
  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
	  constraint_locked = false;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(Reify);
    D_INFO(1,DI_REIFY,"Propagation Start");
    
    if(constraint_locked)
      return;
	  
    if(i == -99998 || i == -99999)
    {
	    constraint_locked = true;
  	  getQueue(stateObj).pushSpecialTrigger(this);
  	  return;
    }
    
    if(full_propagate_called)
    {
      D_ASSERT(rar_var.isAssigned());
      if(rar_var.getAssignedValue() == 1)
        { if(i%2 == 0) poscon->propagate(i/2, domain); }
      else
        { if(i%2 == 1) negcon->propagate((i-1)/2, domain); }
      return;
    }

    if(i%2 == 0)
    { 
      if(poscon->check_unsat(i/2, domain)) 
      { 
        D_INFO(1,DI_REIFY,"Constraint False");
        rar_var.propagateAssign(false);
      }
    }
    else
    { 
      if(negcon->check_unsat((i-1)/2,domain)) 
      {
        D_INFO(1,DI_REIFY,"Constraint True");
        rar_var.propagateAssign(true);
      }
    }
  }

  virtual void full_propagate()
  {
    if(poscon->full_check_unsat())
    {
      D_INFO(1,DI_REIFY,"Pos full_check_unsat true!");
      rar_var.propagateAssign(false);
    }

    if(negcon->full_check_unsat())
    {
      D_INFO(1,DI_REIFY,"False full_check_unsat true!");
      rar_var.propagateAssign(true);
    }

    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue() > 0)
        poscon->full_propagate();
      else
        negcon->full_propagate();
      full_propagate_called = true;
    }
  }
};

template<typename BoolVar>
reify<BoolVar>*
reifyCon(StateObj* stateObj, AbstractConstraint* c, BoolVar var)
{ return new reify<BoolVar>(stateObj, &*c, var); }

#endif
