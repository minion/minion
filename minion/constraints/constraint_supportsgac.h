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

// The algorithm iGAC or short-supports-gac
// i.e. the short supports algrithm.


template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint
{
    virtual string constraint_name()
    {
        return "ShortSupportsGAC";
    }
    
    // Element of doubly-linked list to store supports
    class supElement
    {
        supElement* next;
        supElement* prev;
        vector<pair<int, int> >* support;
    }
    
    VarArray vars;
    int numvals;
    int dom_min;
    
    // Counters
    int supports;   // 0 to rd.  
    vector<int> supportsPerVar;
    vector<vector<int> > supportsPerLit;
    
    // 2d array (indexed by var then val) of list of supports.
    vector<vector<vector<pair<int,int> >* > >  supportListPerLit;
    
    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    vector<vector<int> > zeroVals;
    
    // Partition of variables by number of supports.
    vector<int> varsPerSupport;    // Permutation of the variables
    vector<int> varsPerSupInv;   // Inverse mapping of the above.
    
    vector<int> supportNumPtrs;   // rd+1 indices into varsPerSupport representing the partition 
    
    /*
         MoveableArray<num> flow_s_var;
         vector<MoveableArray<num> > flow_var_val;
         MoveableArray<num> flow_val_t;
         Reversible<num> totalflow;
    */
    
    ShortSupportsGAC(StateObj* _stateObj, const VarArray& _var_array) : vars(_var_array)
    {
        /*
            flow_s_var=getMemory(stateObj).backTrack().template requestArray<num>(numvars);
            flow_val_t=getMemory(stateObj).backTrack().template requestArray<num>(numvals);
            for(int i=0; i<numvars; i++)
            {
                flow_var_val.push_back(getMemory(stateObj).backTrack().template requestArray<num>(numvals));
            }
        */
        int dom_max=vars[0].getMax();
        dom_min=vars[0].getMin();
        for(int i=1; i<vars.size(); i++) {
            if(vars[i].getMin()<dom_min) dom_min=vars[i].getMin();
            if(vars[i].getMax()>dom_max) dom_max=vars[i].getMax();
        }
        numvals=dom_max-dom_min+1;
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(vars.size(), 0);
        supportsPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) supportsPerLit[i].resize(numvals, 0);
        
        supportListPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) supportListPerLit[i].resize(numvals, 0);  // null.
        
        zeroVals.resize(vars.size());
        for(int i=0; i<vars.size(); i++) zeroVals[i].reserve(numvals);  // reserve the maximum length.
        
        
        // Partition
        varsPerSupport.resize(vars.size());
        varsPerSupInv.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            varsPerSupport[i]=i;
            varsPerSupInv[i]=i;
        }
        
        // Start with 1 cell in partition, for 0 supports. 
        supportNumPtrs.resize(vars.size()*numvals+1);
        supportNumPtrs[0]=0;
        for(int i=1; i<supportNumPtrs.size(); i++) supportNumPtrs[i]=vars.size();
        
        
    }
    
    virtual triggerCollection setup_internal()
    {
        triggerCollection t;
        int array_size = vars.size();
        for(int i = 0; i < array_size; ++i)
          t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
        return t;
    }
    
    void partition_swap(int xi, int xj)
    {
        if(xi != xj) {
            varsPerSupport[varsPerSupInv[xj]]=xi;
            varsPerSupport[varsPerSupInv[xi]]=xj;
            int temp=varsPerSupInv[xi];
            varsPerSupInv[xi]=varsPerSupInv[xj];
            varsPerSupInv[xj]=temp;
        }
    }
    
    void print_partition() {
        for(int i=0; i<supportNumPtrs.size()-1; i++) {
            cout << "Variables with " << i << " supports:" << endl;
            for(int j=supportNumPtrs[i]; j<supportNumPtrs[i+1]; j++) {
                cout << varsPerSupport[j] << " ";
            }
            cout <<endl;
        }
    }
    
    void findSupports()
    {
        // For each variable where the number of supports is equal to the total...
        for(int i=supportNumPtrs[supports]; i<supportNumPtrs[supports+1]; i++) {
            int var=varsPerSupport[i];
            while(zeroVals[var].size()!=0) {
                int val=zeroVals[var].back(); zeroVals[var].pop_back();
                if(vars[var].inDomain(val) && supportsPerLit[var][val-dom_min]==0) {
                    // val has no support. Find a new one. 
                    vector<pair<int, int> >* sup=findNewSupport(var, val);
                    if(sup==0) {
                        vars[var].removeFromDomain(val);
                    }
                    else {
                        supports++;
                        for(int j=0; j< (*sup).size(); j++) {
                            pair<int, int> lit=(*sup)[j];
                            
                            supportListPerLit[lit.first][lit.second-dom_min].push_back(sup);
                            supportsPerLit[lit.first][lit.second-dom_min]++;
                            supportsPerVar[lit.first]++;
                            
                            // swap lit.first to the end of its cell.
                            partition_swap(lit.first, varsPerSupport[supportNumPtrs[supportsPerVar[lit.first]]-1]);
                            // Move the boundary so lit.first is now in the higher cell.
                            supportNumPtrs[supportsPerVar[lit.first]]--;
                        }
                        // supports has changed and so has supportNumPtrs so start again. 
                        // Tail recursion might be optimised?
                        findSupports();
                        return;
                    }
                }
            }
        }
    }
    
    
    
    
  typedef typename VarArray::value_type VarRef;
  virtual AbstractConstraint* reverse_constraint()
  { // w-or of pairwise equality.
      
      /// solely for reify exps
      return new CheckAssignConstraint<VarArray, GacAlldiffConstraint2>(stateObj, var_array, *this);
      
      vector<AbstractConstraint*> con;
      for(int i=0; i<var_array.size(); i++)
      {
          for(int j=i+1; j<var_array.size(); j++)
          {
              EqualConstraint<VarRef, VarRef>* t=new EqualConstraint<VarRef, VarRef>(stateObj, var_array[i], var_array[j]);
              con.push_back((AbstractConstraint*) t);
          }
      }
      return new Dynamic_OR(stateObj, con);
  }
  
  smallset to_process;  // set of vars to process.
  
  
  virtual void propagate(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<var_array.size());
    
    #ifdef STAGED
    if(var_array[prop_var].isAssigned())
    {
        int assignedval=var_array[prop_var].getAssignedValue();
        for(int i=0; i<numvars; i++)
        {
            if(i!=prop_var && var_array[i].inDomain(assignedval))
            {
                var_array[i].removeFromDomain(assignedval);
                //adjlist_remove(i, assignedval);
            }
        }
    }
    #endif
    
    if(!to_process.in(prop_var))
    {
        to_process.insert(prop_var);
    }
    
    if(!constraint_locked)
    {
        #ifdef SPECIALQUEUE
        constraint_locked = true;
        getQueue(stateObj).pushSpecialTrigger(this);
        #else
        incremental_prop();
        //do_initial_prop();
        #endif
    }
  }
  
  virtual void special_unlock() { constraint_locked = false; to_process.clear(); }
  virtual void special_check()
  {
    constraint_locked = false;
    
    if(getState(stateObj).isFailed())
    {
        to_process.clear();
        return;
    }
    
    incremental_prop();
    to_process.clear();
  }
  
  
  
  virtual BOOL full_check_unsat()
  { 
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
    {
      if(var_array[i].isAssigned())
      {
      
        for(int j = i + 1; j < v_size; ++j)
        {
          if(var_array[j].isAssigned())
          {
            if(var_array[i].getAssignedValue() == var_array[j].getAssignedValue())
              return true;
          }
        }
        
      }
    }
    
    return false;
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    int v_size = var_array.size();
    if(!var_array[i].isAssigned()) return false;
    
    DomainInt assign_val = var_array[i].getAssignedValue();
    for(int loop = 0; loop < v_size; ++loop)
    {
      if(loop != i)
      {
        if(var_array[loop].isAssigned() && 
           var_array[loop].getAssignedValue() == assign_val)
        return true;
      }
    }
    return false;
  }
  
  virtual void full_propagate()
  {
        // update the adjacency lists. and place dts
        for(int i=dom_min; i<=dom_max; i++)
        {
            for(int j=0; j<adjlistlength[i-dom_min+numvars]; j++)
            {
                int var=adjlist[i-dom_min+numvars][j];
                if(!var_array[var].inDomain(i))
                {
                    adjlist_remove(var, i);
                    j--; // stay in the same place, dont' skip over the 
                    // value which was just swapped into the current position.
                }
                else
                {
                    // arranged in blocks for each variable, with numvals triggers in each block
                    //DynamicTrigger* mydt= dt+(var*numvals)+(i-dom_min);
                    //var_array[var].addDynamicTrigger(mydt, DomainRemoval, i);
                }
            }
        }
        
        //check_adjacency_lists();
        
      to_process.clear();
      
      do_initial_prop();
  }
  
    void check_adjacency_lists()
    {
        for(int var=0; var<numvars; var++)
        {
            for(int val=dom_min; val<=dom_max; val++)
            {
                if(var_array[var].inDomain(val))
                {
                    D_ASSERT(adjlistpos[var][val-dom_min]<adjlistlength[var]);
                    D_ASSERT(adjlistpos[val-dom_min+numvars][var]<adjlistlength[val-dom_min+numvars]);
                }
                else
                {
                    D_ASSERT(adjlistpos[var][val-dom_min]>=adjlistlength[var]);
                    D_ASSERT(adjlistpos[val-dom_min+numvars][var]>=adjlistlength[val-dom_min+numvars]);
                }
            }
        }
    }
    
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
      D_ASSERT(array_size == var_array.size());
      for(int i=0;i<array_size;i++)
        for( int j=i+1;j<array_size;j++)
          if(v[i]==v[j]) return false;
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
    
    
};  // end of class


