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
  Constraint* poscon;
  BoolVar rar_var;
  bool constraint_locked;
  
  reify_true(Constraint* _poscon, BoolVar v) : poscon(_poscon), rar_var(v),
											   constraint_locked(false)
  { }
  
  virtual Constraint* reverse_constraint()
  { D_FATAL_ERROR("You can't reverse a reified Constraint!"); }
  
  virtual bool check_assignment(vector<int> v)
  {
    int back_val = v.back();
    v.pop_back();
    if(back_val)
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
      triggers.push_back(postrig[i]);
    }
    triggers.push_back(make_trigger(rar_var, Trigger(this, -99999), LowerBound));
    return triggers;
  }
  
  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
	poscon->full_propogate();
	constraint_locked = false;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
    D_INFO(1,DI_REIFY,"Propogation Start");
    if(constraint_locked)
	  return;
	  
	if(i == -99999)
    {
      D_INFO(1,DI_REIFY,"Full Pos Propogation");
	  constraint_locked = true;
	  Controller::push_special_trigger(this);
	  //poscon->full_propogate();
      return;
    }
    
    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue())
      { poscon->propogate(i, domain); }
    }
  }
  
  virtual void full_propogate()
  {
    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue())
	    poscon->full_propogate();
    }
  }
};


template<typename BoolVar>
reify_true<BoolVar>*
truereifyCon(Constraint* c, BoolVar var)
{ return new reify_true<BoolVar>(&*c, var); }

