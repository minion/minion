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

template<typename VarArray>
struct GadgetConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Gadget"; }
	
  VarArray var_array;
  
  vector<AnyVarRef> construction_vars;
  
  shared_ptr<CSPInstance> gadget_instance;
  shared_ptr<StateObj> gadget_stateObj;
  
  bool constraint_locked;
  
  GadgetConstraint(StateObj* _stateObj, const VarArray& _vars, shared_ptr<CSPInstance> _gadget) : 
  Constraint(_stateObj), var_array(_vars), gadget_instance(_gadget),
  gadget_stateObj(shared_ptr<StateObj>(new StateObj)),
  constraint_locked(false)
  { 
    BuildCSP(&*stateObj, *gadget_instance); 
    construction_vars.reserve(gadget_instance->constructionSite.size());
    for(int i = 0; i < gadget_instance->constructionSite.size(); ++i)
      construction_vars.push_back(get_AnyVarRef_from_Var(stateObj, gadget_instance->constructionSite[i]));
    if(construction_vars.size() != var_array.size())
      D_FATAL_ERROR("Gadgets construction site");
  }
  
  virtual Constraint* reverse_constraint()
  {
    cerr << "You can't reverse a gadget Constraint!";
    FAIL_EXIT();
  }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    for(int i = 0; i < var_array.size(); ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
	return t;
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  { D_FATAL_ERROR(".."); }
  
  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(var_array[i]);
    return vars;
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
	constraint_locked = false;
  }
  
  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
	constraint_locked = false;
    full_propagate();
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
    PROP_INFO_ADDONE(Gadget);
	if(constraint_locked)
	  return;

    constraint_locked = true;
    getQueue(stateObj).pushSpecialTrigger(this);
  }
  
  virtual void full_propagate()
  { 
    for(int i = 0; i < var_array.size(); ++i)
    {
      DomainInt min_val = var_array[i].getMin();
      DomainInt max_val = var_array[i].getMax();
      construction_vars[i].setMin(min_val);
      construction_vars[i].setMax(max_val);
      
      for(int j = min_val + 1; j <max_val; ++j)
        if(!var_array[i].inDomain(j))
          construction_vars[i].removeFromDomain(j);
    }
  
  }
};

template<typename VarArray>
GadgetConstraint<VarArray>*
gadgetCon(StateObj* stateObj, const VarArray& vars, ConstraintBlob& blob)
{ return new GadgetConstraint<VarArray>(stateObj, vars, blob.gadget); }

#ifdef REENTER

template<typename Vars>
Constraint*
BuildCT_GADGET(StateObj* stateObj, const Vars& vars, BOOL reify, const BoolVarRef& reifyvar, ConstraintBlob& blob)
{
  if(reify)
  {  D_FATAL_ERROR("Can't reify gadget constraints. Sorry."); }
  else
    return gadgetCon(stateObj, vars, blob);
}

#else

template<typename Vars>
Constraint*
BuildCT_GADGET(StateObj* stateObj, const Vars& vars, BOOL reify, const BoolVarRef& reifyvar, ConstraintBlob& blob)
{ D_FATAL_ERROR("This constraint requires REENTER support."); }
#endif
