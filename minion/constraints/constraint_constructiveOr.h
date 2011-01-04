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

// An implementation of constructive disjunction based on the gadget constraint.
// The disjunction is hard-coded in here, it only reads variables from the input file.

// Variables are duplicated into the gadget for each disjunct.

#ifndef CONSTRAINT_CONSTRUCTIVEOR_H
#define CONSTRAINT_CONSTRUCTIVEOR_H

#include "../preprocess.h"

template<typename VarArray>
struct ConstructiveOr : public AbstractConstraint
{
  virtual string constraint_name()
  { return "ConstructiveOr"; }
    
  VarArray var_array;
  
  
  
  vector<StateObj*> gadget_stateObj;   //  stateObj for each disjunct.
  
  vector<vector<AnyVarRef> > inner_vars;
  
  bool constraint_locked;
  
  ConstructiveOr(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj), var_array(_var_array),
  constraint_locked(false)
  { 
    make_disjunct_csps();
  }
  
  virtual ~ConstructiveOr()
  { for(int i=0; i<gadget_stateObj.size(); i++) delete gadget_stateObj[i]; }
  
  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "You can't reverse a constructive disjunction constraint!";
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
    cout << "Constructive OR Assignment:" << v << endl;
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
  
  virtual void propagate(int i, DomainDelta domain)
  {
    /*PROP_INFO_ADDONE(Gadget);
    if(constraint_locked)
      return;

    constraint_locked = true;
    getQueue(stateObj).pushSpecialTrigger(this);*/
    do_prop();
  }
  
  virtual void full_propagate()
  {
    /*if(getState(gadget_stateObj).isFailed())
    {
      getState(stateObj).setFailed(true);
      return;
    }*/
    do_prop();
  }
  
  // Non-incremental just to get it working.
  void do_prop()
  { 
    
    
    
    //D_ASSERT(!getState(gadget_stateObj).isFailed());
    
    for(int i=0; i<gadget_stateObj.size(); i++)
        Controller::world_push(gadget_stateObj[i]);
    
    for(int i = 0; i < var_array.size(); ++i)
    {
      DomainInt min_val = var_array[i].getMin();
      DomainInt max_val = var_array[i].getMax();
      for(int j=0; j<inner_vars.size(); j++) {
          inner_vars[j][i].setMin(min_val);
          inner_vars[j][i].setMax(max_val);
      }
      
      for(int j = min_val + 1; j < max_val; ++j)
      if(!var_array[i].inDomain(j)) {
          for(int k=0; k<inner_vars.size(); k++) {
              inner_vars[k][i].removeFromDomain(j);
          }
      }
    }
    
    for(int i=0; i<gadget_stateObj.size(); i++) {
        PropogateCSP(gadget_stateObj[i], PropLevel_GAC, inner_vars[i]);
    }
    
    for(int i=0; i<var_array.size(); i++) {
        for(int val=var_array[i].getMin(); val<=var_array[i].getMax(); val++) {
            // Search for a disjunct supporting val.
            bool supported=false;
            for(int j=0; j<gadget_stateObj.size(); j++) {
                if(!(getState(gadget_stateObj[j]).isFailed()) && inner_vars[j][i].inDomain(val)) {
                    supported=true; break;
                }
            }
            if(!supported) {
                var_array[i].removeFromDomain(val);
            }
        }
    }
    
    for(int i=0; i<gadget_stateObj.size(); i++) {
        Controller::world_pop(gadget_stateObj[i]);
    }
    
    
  }
  
  //////////////////////////////////////////////////////////////////////////////
  // Just a simple <= for now,  < \/ =.
    void make_disjunct_csps()
    {
        Bounds var1bounds(var_array[0].getInitialMin(), var_array[0].getInitialMax());
        Bounds var2bounds(var_array[1].getInitialMin(), var_array[1].getInitialMax());
        
        
        // For each disjunct, make a CSPInstance and then call BuildCSP for it.
        StateObj* diseq_stateObj=new StateObj();
        gadget_stateObj.push_back(diseq_stateObj);
        
        StateObj* abs_stateObj=new StateObj();
        gadget_stateObj.push_back(abs_stateObj);
        
        CSPInstance cspi;
        cspi.vars.discrete.push_back(make_pair(0, var1bounds));
        cspi.vars.discrete.push_back(make_pair(1, var2bounds));
        
        Var var1(VAR_DISCRETE, 0);
        Var var2(VAR_DISCRETE, 1);
        
        vector<vector<Var> > cvars;
        cvars.resize(2);
        cvars[0].push_back(var1);
        cvars[1].push_back(var2);
        
        
        ConstraintBlob cblob(&(constraint_list[11]), cvars);  // diseq.
        
        cspi.constraints.push_back(cblob);
        
        BuildCSP(diseq_stateObj, cspi);
        
        // Stick an abs in there instead
        ConstraintBlob cblob2(&(constraint_list[17]), cvars);  // abs.
        
        cspi.constraints.pop_back();
        cspi.constraints.push_back(cblob);
        
        BuildCSP(abs_stateObj, cspi);
        
        inner_vars.resize(2);  // two constraints.
        
        // lt
        inner_vars[0].push_back(get_AnyVarRef_from_Var(diseq_stateObj, var1));
        inner_vars[0].push_back(get_AnyVarRef_from_Var(diseq_stateObj, var2));
        
        // eq
        inner_vars[1].push_back(get_AnyVarRef_from_Var(abs_stateObj, var1));
        inner_vars[1].push_back(get_AnyVarRef_from_Var(abs_stateObj, var2));
        
        
        
        /*
        
        construction_vars.reserve(gadget_instance->constructionSite.size());
        for(int i = 0; i < gadget_instance->constructionSite.size(); ++i)
            construction_vars.push_back(get_AnyVarRef_from_Var(gadget_stateObj, gadget_instance->constructionSite[i]));
        if(construction_vars.size() != var_array.size())
            INPUT_ERROR("Gadgets construction site is incorrect size");*/
        
    }
    
    
};

#endif
