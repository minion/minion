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


// Note: The whole constraint_locked thing is for the following case:
// Consider the following events are on the queue:
// "rareify boolean is assigned, Y is assigned"
// Now "rareify boolean is assigned" causes full_propogate to be called for
// the constraint. It will set up it's data structures based on the current
// assignment. Then later it will be given Y is assigned, but have already
// possibly used that. Confusion follows. Therefore when we want to propogate
// the function, we "lock" it until the queue empties, then start propogating
// the constraint.

template<typename BoolVar>
struct reify : public Constraint
{
  virtual string constraint_name()
  { return "Reify:" + poscon->constraint_name(); }
	
  Constraint* poscon;
  Constraint* negcon;
  BoolVar rar_var;
  
  // These two variables are only used in special cases.
  BOOL constraint_locked;
  BOOL value_assigned;
  
  reify(Constraint* _poscon, BoolVar v) : poscon(_poscon), rar_var(v),
										  constraint_locked(false)
  { negcon = poscon->reverse_constraint();}
  
  virtual Constraint* reverse_constraint()
  {
    cerr << "You can't reverse a reified Constraint!";
    FAIL_EXIT();
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    // This function is very slow compared to what it could be.
	// This is unfortunate, but hopefully not important.
	int vec1size = poscon->get_vars().size();
    DomainInt back_val = v.back();
    v.pop_back();
    if(back_val != 0)
	{
	  v.erase(v.begin() + vec1size, v.end());
      return poscon->check_assignment(v);
	}
    else
	{
	  v.erase(v.begin(), v.begin() + vec1size);
      return negcon->check_assignment(v);
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
	if(value_assigned)
	  poscon->full_propogate();
    else
	  negcon->full_propogate();
  }
  
  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
	constraint_locked = false;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
	PROP_INFO_ADDONE(Reify);
    D_INFO(1,DI_REIFY,"Propogation Start");
	if(constraint_locked)
	  return;
	  
    if(i == -99998 || i == -99999)
    {
	  constraint_locked = true;
	  Controller::push_special_trigger(this);
	  
      if(i==-99999)
      {
		D_INFO(1,DI_REIFY,"Full Pos Propogation");
		value_assigned = true;
		//poscon->full_propogate();
      }
      else
      {
		D_INFO(1,DI_REIFY,"Full Neg Propogation");
		value_assigned = false;
		//negcon->full_propogate();
      }
      return;
    }
    
    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue() == 1)
      { if(i%2 == 0) poscon->propogate(i/2, domain); }
      else
      { if(i%2 == 1) negcon->propogate((i-1)/2, domain); }
    }
    else
    {
      if(i%2 == 0)
      { 
		if(poscon->check_unsat(i/2, domain)) 
		{ 
		  D_INFO(1,DI_REIFY,"Constraint False");
		  rar_var.uncheckedAssign(false);
		}
      }
      else
      { 
		if(negcon->check_unsat((i-1)/2,domain)) 
		{
		  D_INFO(1,DI_REIFY,"Constraint True");
		  rar_var.uncheckedAssign(true);
		}
      }
    }
  }
  
  virtual void full_propogate()
  {
    if(poscon->full_check_unsat())
	{
	  D_INFO(1,DI_REIFY,"Pos full_check_unsat true!");
      rar_var.propogateAssign(false);
	}
	
    if(negcon->full_check_unsat())
	{
	  D_INFO(1,DI_REIFY,"False full_check_unsat true!");
      rar_var.propogateAssign(true);
	}
    
    if(rar_var.isAssigned())
    {
      if(rar_var.getAssignedValue() == 1)
		poscon->full_propogate();
      else
		negcon->full_propogate();
    }
  }
};

template<typename BoolVar>
reify<BoolVar>*
reifyCon(Constraint* c, BoolVar var)
{ return new reify<BoolVar>(&*c, var); }


