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

// Whether to propagate incrementally.
#define CORINCREMENTAL true

template<typename VarArray>
struct ConstructiveOr : public AbstractConstraint, Backtrackable
{
  virtual string constraint_name()
  { return "ConstructiveOr"; }
    
  VarArray var_array;
  
  vector<StateObj*> gadget_stateObj;   //  stateObj for each disjunct.
  
  vector<vector<AnyVarRef> > inner_vars;
  
  int numvals;
    int dom_min;
    int dom_max;
    
    
    ConstructiveOr(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj), var_array(_var_array)
    {
        if(CORINCREMENTAL) {
            getState(stateObj).getGenericBacktracker().add(this);
        }
        make_disjunct_csps();
        
        failed_stack.push_back(-1); // marker. May not be necessary.
        
        dom_max=var_array[0].getInitialMax();
        dom_min=var_array[0].getInitialMin();
        for(int i=1; i<var_array.size(); i++) {
            if(var_array[i].getInitialMin()<dom_min) dom_min=var_array[i].getInitialMin();
            if(var_array[i].getInitialMax()>dom_max) dom_max=var_array[i].getInitialMax();
        }
        numvals=dom_max-dom_min+1;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //   Backtracking functions.
    
    // Stack of child CSPS that have failed, and need to be un-failed when backtracking.
    vector<int> failed_stack;
    
    void mark() {
        failed_stack.push_back(-1);  // marker.
        
        for(int i=0; i<gadget_stateObj.size(); i++) {
            if(!getState(gadget_stateObj[i]).isFailed()) {
                Controller::world_push(gadget_stateObj[i]);
            }
        }
    }
    
    void pop() {
        // Backtrack each of the child CSPs.
        for(int i=0; i<gadget_stateObj.size(); i++) {
            if(!getState(gadget_stateObj[i]).isFailed()) {
                Controller::world_pop(gadget_stateObj[i]);
            }
        }
        
        while(failed_stack.back()!=-1) {
            D_ASSERT(getState(gadget_stateObj[failed_stack.back()]).isFailed());
            getState(gadget_stateObj[failed_stack.back()]).setFailed(false);
            failed_stack.pop_back();
        }
        // pop the marker.
        failed_stack.pop_back();
        
    }
    
  
  
  virtual ~ConstructiveOr()
  { for(int i=0; i<gadget_stateObj.size(); i++) delete gadget_stateObj[i]; }
  
  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "You can't reverse a constructive disjunction constraint!";
    FAIL_EXIT();
  }
  
  int dynamic_trigger_count() { 
      if(CORINCREMENTAL)
          return var_array.size()*numvals;
      else
          return 0;
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    if(!CORINCREMENTAL) {
        for(int i = 0; i < var_array.size(); ++i) {
            t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
        }
    }
    return t;
  }
  
  inline void attach_trigger(int var, int val) {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+(var*numvals)+(val-dom_min);
      D_ASSERT(!dt->isAttached());
      
      var_array[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  { 
    //cout << "Constructive OR Assignment:" << v[0] << "," << v[1] << endl;
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
  
  virtual void propagate(int i, DomainDelta domain)
  {
    /*PROP_INFO_ADDONE(Gadget);
    if(constraint_locked)
      return;

    constraint_locked = true;
    getQueue(stateObj).pushSpecialTrigger(this);*/
    D_ASSERT(!CORINCREMENTAL);
    do_prop();
  }
  
  virtual void propagate(DynamicTrigger* dt)
  {
      D_ASSERT(CORINCREMENTAL);
      
      // ALSO SET UP THE DYNAMIC TRIGGERS...
      int pos=dt-dynamic_trigger_start();
      int var=pos/numvals;
      int val=pos-(var*numvals)+dom_min;
      
      // Prune val in all child CSPs that have not failed.
      for(int i=0; i<gadget_stateObj.size(); i++) {
          if(!getState(gadget_stateObj[i]).isFailed()) {
              inner_vars[i][var].removeFromDomain(val);
          }
      }
      
      do_prop_incremental();
  }
  
  
  virtual void full_propagate()
  {
      if(CORINCREMENTAL) {
          
          // Copy domains
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
          
          do_prop_incremental();
          
          for(int var=0; var<var_array.size(); var++) {
              for(int val=dom_min; val<=dom_max; val++) {
                  if(var_array[var].inDomain(val)) {
                      attach_trigger(var, val);
                  }
              }
          }
          
      }
      else {
          do_prop();
      }
  }
  
  // Non-incremental just to get it working.
  void do_prop()
  { 
    
    for(int i=0; i<gadget_stateObj.size(); i++) {
        if(getState(gadget_stateObj[i]).isFailed()) {
            getState(gadget_stateObj[i]).setFailed(false);
        }
        
        
        Controller::world_push(gadget_stateObj[i]);
    }
    
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
        //cout << "About to propagate CSP "<< i << endl;
        //cout << "x0:" << inner_vars[i][0].getMin() << "," <<inner_vars[i][0].getMax()<<endl;
        //cout << "x1:" << inner_vars[i][1].getMin() << "," <<inner_vars[i][1].getMax()<<endl;
        
        PropogateCSP(gadget_stateObj[i], PropLevel_GAC, inner_vars[i]);
        
    }
    
    for(int i=0; i<var_array.size(); i++) {
        for(int val=var_array[i].getMin(); val<=var_array[i].getMax(); val++) {
            // Search for a disjunct supporting val.
            bool supported=false;
            
            for(int j=0; j<gadget_stateObj.size(); j++) {
                //if(getState(gadget_stateObj[j]).isFailed()) cout << "Child CSP " << j << " failed."<<endl;
                
                if(!(getState(gadget_stateObj[j]).isFailed()) && inner_vars[j][i].inDomain(val)) {
                    supported=true; break;
                }
            }
            if(!supported) {
                //cout << "Var,val not supported: "<< i<< "," <<val<<endl;
                var_array[i].removeFromDomain(val);
            }
        }
    }
    
    for(int i=0; i<gadget_stateObj.size(); i++) {
        Controller::world_pop(gadget_stateObj[i]);
    }
    
    
  }
  
  
  void do_prop_incremental()
  { 
    // Assume domains of child CSPs have already been updated by propagate function.
    // Propagate the child CSPs
    
    for(int i=0; i<gadget_stateObj.size(); i++) {
        if(!getState(gadget_stateObj[i]).isFailed()) {
            PropogateCSP(gadget_stateObj[i], PropLevel_GAC, inner_vars[i]);
            
            if(getState(gadget_stateObj[i]).isFailed()) {
                failed_stack.push_back(i);   // Need to reset failed flag when backtracking.
            }
        }
    }
    
    for(int i=0; i<var_array.size(); i++) {
        for(int val=var_array[i].getMin(); val<=var_array[i].getMax(); val++) {
            if(var_array[i].inDomain(val)) {
                // Search for a disjunct supporting val.
                bool supported=false;
                
                for(int j=0; j<gadget_stateObj.size(); j++) {
                    //if(getState(gadget_stateObj[j]).isFailed()) cout << "Child CSP " << j << " failed."<<endl;
                    
                    if(!(getState(gadget_stateObj[j]).isFailed()) && inner_vars[j][i].inDomain(val)) {
                        supported=true; break;
                    }
                }
                if(!supported) {
                    //cout << "Var,val not supported: "<< i<< "," <<val<<endl;
                    var_array[i].removeFromDomain(val);
                }
            }
        }
    }
    
  }
  
  
  //////////////////////////////////////////////////////////////////////////////
  // diseq or abs.
    void make_disjunct_csps()
    {
        Bounds var1bounds(var_array[0].getInitialMin(), var_array[0].getInitialMax());
        Bounds var2bounds(var_array[1].getInitialMin(), var_array[1].getInitialMax());
        vector<int> var1dom; var1dom.push_back(var_array[0].getInitialMin()); var1dom.push_back(var_array[0].getInitialMax());
        vector<int> var2dom; var2dom.push_back(var_array[1].getInitialMin()); var2dom.push_back(var_array[1].getInitialMax());
        
        
        // For each disjunct, make a CSPInstance and then call BuildCSP for it.
        StateObj* diseq_stateObj=new StateObj();
        gadget_stateObj.push_back(diseq_stateObj);
        
        StateObj* abs_stateObj=new StateObj();
        gadget_stateObj.push_back(abs_stateObj);
        
        CSPInstance cspi;
        
        Var var1 = cspi.vars.getNewVar(VAR_DISCRETE, var1dom);
        cspi.vars.addSymbol("x1", var1);
        cspi.all_vars_list.push_back(make_vec(var1));
        
        Var var2 = cspi.vars.getNewVar(VAR_DISCRETE, var2dom);
        cspi.vars.addSymbol("x2", var2);
        cspi.all_vars_list.push_back(make_vec(var2));
        
        
        
        vector<vector<Var> > cvars;
        cvars.resize(2);
        cvars[0].push_back(var1);
        cvars[1].push_back(var2);
        
        
        ConstraintBlob cblob(&(constraint_list[11]), cvars);  // diseq.
        
        cspi.add_constraint(cblob);
        
        BuildCSP(diseq_stateObj, cspi);
        
        // Stick an abs in there instead
        ConstraintBlob cblob2(&(constraint_list[17]), cvars);  // abs.
        
        cspi.add_constraint(cblob2);
        
        BuildCSP(abs_stateObj, cspi);
        
        inner_vars.resize(2);  // two constraints.
        
        // lt
        inner_vars[0].push_back(get_AnyVarRef_from_Var(diseq_stateObj, var1));
        inner_vars[0].push_back(get_AnyVarRef_from_Var(diseq_stateObj, var2));
        
        // eq
        inner_vars[1].push_back(get_AnyVarRef_from_Var(abs_stateObj, var1));
        inner_vars[1].push_back(get_AnyVarRef_from_Var(abs_stateObj, var2));
        
    }
    
    
};

#endif
