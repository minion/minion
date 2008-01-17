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



template<typename BoolVar>
struct reify_true : public Constraint
{
  StateObj* stateObj;
  virtual string constraint_name()
  { return "ReifyTrue:" + poscon->constraint_name(); }
  
  Constraint* poscon;
  BoolVar rar_var;
  BOOL constraint_locked;
  
  reify_true(StateObj* _stateObj, Constraint* _poscon, BoolVar v) : Constraint(_stateObj), stateObj(_stateObj), 
                                                                    poscon(_poscon), 
                                                                    rar_var(v), constraint_locked(false)
  { }
  
  virtual Constraint* reverse_constraint()
  { D_FATAL_ERROR("You can't reverse a reified Constraint!"); }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    DomainInt back_val = v.back();
    v.pop_back();
    if(back_val != 0)
      return poscon->check_assignment(v);
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
    D_INFO(2,DI_REIFY,"Setting up rarification");
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
  }
  
  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
	constraint_locked = false;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
	PROP_INFO_ADDONE(ReifyTrue);
    D_INFO(1,DI_REIFY,"Propagation Start");
    if(constraint_locked)
	  return;
	  
	if(i == -99999)
    {
      D_INFO(1,DI_REIFY,"Full Pos Propagation");
	  constraint_locked = true;
	  getQueue(stateObj).pushSpecialTrigger(this);
	  //poscon->full_propagate();
      return;
    }
    
    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue() == 1)
      { poscon->propagate(i, domain); }
    }
  }
  
  virtual void full_propagate()
  {
    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue() != 0)
	    poscon->full_propagate();
    }
  }
};


template<typename BoolVar>
reify_true<BoolVar>*
truereifyCon(StateObj* stateObj, Constraint* c, BoolVar var)
{ return new reify_true<BoolVar>(stateObj, &*c, var); }

// Just a placeholder.
template<typename BoolVar>
DynamicConstraint*
truereifyCon(StateObj* stateObj, DynamicConstraint* c, BoolVar var)
{ 
  cerr << "Reification is not supported on dynamic constraints. Sorry." << endl;
  exit(1);
}



