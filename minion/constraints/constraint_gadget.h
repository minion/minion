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

#ifndef CONSTRAINT_GADGET_H
#define CONSTRAINT_GADGET_H

#include "../preprocess.h"

template<typename VarArray>
struct GadgetConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Gadget"; }
	
  VarArray var_array;
  
  vector<AnyVarRef> construction_vars;
  
  shared_ptr<CSPInstance> gadget_instance;
  StateObj* gadget_stateObj;
  PropagationLevel gadget_prop_type;
  
  bool constraint_locked;
  
  GadgetConstraint(StateObj* _stateObj, const VarArray& _vars, shared_ptr<CSPInstance> _gadget, PropagationLevel _proptype) : 
  AbstractConstraint(_stateObj), var_array(_vars), gadget_instance(_gadget),
  gadget_stateObj(new StateObj), gadget_prop_type(_proptype), 
  constraint_locked(false)
  { 
    BuildCSP(gadget_stateObj, *gadget_instance); 
    construction_vars.reserve(gadget_instance->constructionSite.size());
    for(int i = 0; i < gadget_instance->constructionSite.size(); ++i)
      construction_vars.push_back(get_AnyVarRef_from_Var(gadget_stateObj, gadget_instance->constructionSite[i]));
    if(construction_vars.size() != var_array.size())
      INPUT_ERROR("Gadgets construction site is incorrect size");
  }
  
  virtual ~GadgetConstraint()
  { delete gadget_stateObj; }
  
  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "You can't reverse a gadget Constraint!";
    FAIL_EXIT();
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(int i = 0; i < var_array.size(); ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
	return t;
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  { 
    cout << "Gadget Assignment:" << v << endl;
    return true;
  }
  
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
    do_prop();
  }
  
  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
	constraint_locked = false;
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
    if(getState(gadget_stateObj).isFailed())
    {
      getState(stateObj).setFailed(true);
      return;
    }
    do_prop();
  }

  void do_prop()
  { 
    D_ASSERT(!getState(gadget_stateObj).isFailed());
    Controller::world_push(gadget_stateObj);
    
    for(int i = 0; i < var_array.size(); ++i)
    {
      DomainInt min_val = var_array[i].getMin();
      DomainInt max_val = var_array[i].getMax();
      construction_vars[i].setMin(min_val);
      construction_vars[i].setMax(max_val);
      
      for(int j = min_val + 1; j < max_val; ++j)
        if(!var_array[i].inDomain(j))
          construction_vars[i].removeFromDomain(j);
    }
    
    PropogateCSP(gadget_stateObj, gadget_prop_type, construction_vars);

    if(getState(gadget_stateObj).isFailed())
    {
      getState(gadget_stateObj).setFailed(false);
      Controller::world_pop(gadget_stateObj);
      getState(stateObj).setFailed(true);
      return;
    }
        
    for(int i = 0; i < var_array.size(); ++i)
    {
      DomainInt min_val = construction_vars[i].getMin();
      DomainInt max_val = construction_vars[i].getMax();
      var_array[i].setMin(min_val);
      var_array[i].setMax(max_val);
      
      for(int j = min_val + 1; j < max_val; ++j)
      {
        if(!construction_vars[i].inDomain(j))
        { 
          var_array[i].removeFromDomain(j);
        }
      }
    }
    
    Controller::world_pop(gadget_stateObj);
  }
};

template<typename VarArray>
GadgetConstraint<VarArray>*
gadgetCon(StateObj* stateObj, const VarArray& vars, ConstraintBlob& blob)
{ return new GadgetConstraint<VarArray>(stateObj, vars, blob.gadget, blob.gadget_prop_type); }

#ifdef REENTER

template<typename Vars>
AbstractConstraint*
BuildCT_GADGET(StateObj* stateObj, const Vars& vars, ConstraintBlob& blob)
{ return gadgetCon(stateObj, vars, blob); }

#else

template<typename Vars>
AbstractConstraint*
BuildCT_GADGET(StateObj* stateObj, const Vars& vars, ConstraintBlob& blob)
{ INPUT_ERROR("This constraint requires REENTER support."); }
#endif

#endif
