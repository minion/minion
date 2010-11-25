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

template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint, Backtrackable
{
    struct Support {
        vector<Support*> prev;   // Size r -- some entries null.
        vector<Support*> next;   
        
        // Prev and next are indexed by variable. Must be Null if the support does
        // not include that variable. 
        
        vector<pair<int,int> >* literals;
    };
    
    virtual string constraint_name()
    {
        return "ShortSupportsGAC";
    }
    
    StateObj* stateObj;
    VarArray vars;
    
    int numvals;
    int dom_min;
    int dom_max;
    
    // Counters
    int supports;   // 0 to rd.  
    vector<int> supportsPerVar;
    vector<vector<int> > supportsPerLit;
    
    // 2d array (indexed by var then val) of sentinels,
    // at the head of list of supports. 
    // Needs a sentinel at the start so that dlx-style removals work correctly.
    vector<vector<Support> >  supportListPerLit;
    
    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    vector<vector<int> > zeroVals;
    
    // Partition of variables by number of supports.
    vector<int> varsPerSupport;    // Permutation of the variables
    vector<int> varsPerSupInv;   // Inverse mapping of the above.
    
    
    vector<int> supportNumPtrs;   // rd+1 indices into varsPerSupport representing the partition
    
    Support* supportFreeList;       // singly-linked list of spare Support objects.
    vector< vector<pair<int,int> >* > litsFreeList;    // vector of spare vector<pair<int,int>> to use for storing lists of literals.
    
    ////////////////////////////////////////////////////////////////////////////
    // Backtracking mechanism
    
    // When deleting a support, the literal list goes on this stack. It is added back on backtracking. 
    vector<vector<pair<int,int> >* > backtrack_stack_removals;
    
    // When adding a support it goes on this stack. It is removed on backtracking.
    vector<Support*> backtrack_stack_additions; 
    
    void mark() {
        backtrack_stack_removals.push_back(0);  // marker.
        backtrack_stack_additions.push_back(0);
    }
    
    void pop() {
        while(backtrack_stack_removals.back()!=0) {
            vector<pair<int,int> >* litlist=backtrack_stack_removals.back();
            backtrack_stack_removals.pop_back();
            addSupportInternal(litlist);
        }
        while(backtrack_stack_additions.back()!=0) {
            Support* sup=backtrack_stack_additions.back();
            backtrack_stack_additions.pop_back();
            deleteSupportInternal(sup);
        }
        backtrack_stack_removals.pop_back();  // Pop the markers.
        backtrack_stack_additions.pop_back();
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Add and delete support
    
    template<typename listtype>
    Support* addSupport(listtype* litlist)
    {
        Support* newsup=addSupportInternal(litlist);
        backtrack_stack_additions.push_back(newsup);
        return newsup;
    }
    
    template<typename listtype>
    Support* addSupportInternal(listtype* litlist)
    {
        // add a new support given as a vector of literals.
        Support* newsup=getFreeSupport();
        
        newsup->literals->clear();
        for(int i=0; i<litlist->size(); i++) newsup->literals->push_back((*litlist)[i]);
        
        
        int litsize=litlist->size();
        for(int i=0; i<litsize; i++) {
            pair<int, int> temp=(*litlist)[i];
            int var=temp.first;
            int val=temp.second-dom_min;
            
            // Stitch it into supportListPerLit
            newsup->prev[var]= &(supportListPerLit[var][val]);
            newsup->next[var]= supportListPerLit[var][val].next[var];
            supportListPerLit[var][val].next[var]=newsup;
            if(newsup->next[var] != 0)
                newsup->next[var]->prev[var]=newsup;
            
            //update counters
            supportsPerVar[var]++;
            supportsPerLit[var][val]++;
            
            // Update partition
            // swap var to the end of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]]-1]);
            // Move the boundary so var is now in the higher cell.
            supportNumPtrs[supportsPerVar[var]]--;
        }
        supports++;
        
        return newsup;
    }
    
    // Need another stack of sups to be deleted on bt!
    
    void deleteSupport(Support* sup) {
        deleteSupportInternal(sup);
        // Swap the literal list in sup with a spare one.
        // Put the one from sup onto the backtrack stack.
        vector<pair<int,int> >* spare=getFreeLitlist();
        backtrack_stack_removals.push_back(sup->literals);
        sup->literals=spare;
    }
    
    void deleteSupportInternal(Support* sup) {
        D_ASSERT(sup!=0);
        
        // Remove sup from supportListPerLit
        vector<Support*>& prev=sup->prev;
        vector<Support*>& next=sup->next;
        vector<pair<int, int> >& litlist=*(sup->literals);
        
        for(int i=0; i<litlist.size(); i++) {
            int var=litlist[i].first;
            D_ASSERT(prev[var]!=0);
            prev[var]->next[var]=next[var];
            if(next[var]!=0) {
                next[var]->prev[var]=prev[var];
            }
            
            // decrement counters
            supportsPerVar[var]--;
            supportsPerLit[var][litlist[i].second-dom_min]--;
            if(supportsPerLit[var][litlist[i].second-dom_min]==0)
                zeroVals[var].push_back(litlist[i].second);
            
            // Update partition
            // swap var to the start of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]+1]]);
            // Move the boundary so var is now in the lower cell.
            supportNumPtrs[supportsPerVar[var]+1]++;
        }
        supports--;
        
        // Stick it on the free list using next[0] as the next ptr.
        sup->next[0]=supportFreeList;
        supportFreeList=sup;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Ctor
    
    ShortSupportsGAC(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj), 
    stateObj(_stateObj), vars(_var_array)
    {
        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        dom_max=vars[0].getInitialMax();
        dom_min=vars[0].getInitialMin();
        for(int i=1; i<vars.size(); i++) {
            if(vars[i].getInitialMin()<dom_min) dom_min=vars[i].getInitialMin();
            if(vars[i].getInitialMax()>dom_max) dom_max=vars[i].getInitialMax();
        }
        numvals=dom_max-dom_min+1;
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(vars.size(), 0);
        supportsPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) supportsPerLit[i].resize(numvals, 0);
        
        supportListPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            supportListPerLit[i].resize(numvals);  // blank Support objects.
            for(int j=0; j<numvals; j++) {
                supportListPerLit[i][j].next.resize(vars.size());
            }
        }
        
        zeroVals.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            zeroVals[i].reserve(numvals);  // reserve the maximum length.
            for(int j=dom_min; j<=dom_max; j++) zeroVals[i].push_back(j);
        }
        
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
    
    ////////////////////////////////////////////////////////////////////////////
    // 
    void printStructures()
    {
        cout << "PRINTING ALL DATA STRUCTURES" <<endl;
        cout << "supports:" << supports <<endl;
        cout << "supportsPerVar:" << supportsPerVar << endl;
        cout << "supportsPerLit:" << supportsPerLit << endl;
        
        
        
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
    restartloop:
        for(int i=supportNumPtrs[supports]; i<supportNumPtrs[supports+1]; i++) {
            int var=varsPerSupport[i];
            while(zeroVals[var].size()!=0) {
                int val=zeroVals[var].back(); zeroVals[var].pop_back();
                if(vars[var].inDomain(val) && supportsPerLit[var][val-dom_min]==0) {
                    // val has no support. Find a new one. 
                    typedef pair<int,DomainInt> temptype;
                    MAKE_STACK_BOX(newsupportbox, temptype, vars.size());
                    bool foundsupport=findNewSupport(newsupportbox, var, val);
                    
                    if(!foundsupport) {
                        vars[var].removeFromDomain(val);
                    }
                    else {
                        addSupport(&newsupportbox);
                        
                        // supports has changed and so has supportNumPtrs so start again. 
                        // Tail recursion might be optimised?
                        // Should be a goto.
                        goto restartloop;
                        //findSupports();
                        //return;
                    }
                }
            }
        }
    }
    
    void updateCounters(int var, int val) {
        Support* supList = supportListPerLit[var][val-dom_min].next[var];
        //supportListPerLit[var][val-dom_min]=0;
        while(supList != 0) {
            Support* next=supList->next[var];
            deleteSupport(supList);
            supList=next;
        }
    }
    
    
  /*typedef typename VarArray::value_type VarRef;
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
  }*/
  
  
  virtual void propagate(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<vars.size());
    // Really needs triggers on each value, or on the supports. 
    
    cout << "Entered propagate for supportsgac."<< endl;
    
    printStructures();
    
    for(int val=dom_min; val<=dom_max; val++) {
        if(!vars[prop_var].inDomain(val) && supportsPerLit[prop_var][val-dom_min]>0) {
            updateCounters(prop_var, val);
        }
    }
    
    findSupports();
    cout << "Leaving propagate for supportsgac."<< endl;
  }
    
    
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        // a=b or c=d
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()==4);
        int othervar;
        if(var<=1) othervar=1-var;
        else othervar=(var==2? 3: 2);
        
        if(vars[othervar].inDomain(val)) {
            // If can satisfy the equality with var in it
            assignment.push_back(make_pair(var, val));
            assignment.push_back(make_pair(othervar, val));
            return true;
        }
        
        // Otherwise, try to satisfy the other equality.
        if(var<=1) {
            for(int otherval=vars[2].getMin(); otherval<=vars[2].getMax(); otherval++) {
                if(vars[2].inDomain(otherval) && vars[3].inDomain(otherval)) {
                    assignment.push_back(make_pair(2, otherval));
                    assignment.push_back(make_pair(3, otherval));
                    return true;
                }
            }
        }
        else {
            for(int otherval=vars[0].getMin(); otherval<=vars[0].getMax(); otherval++) {
                if(vars[0].inDomain(otherval) && vars[1].inDomain(otherval)) {
                    assignment.push_back(make_pair(0, otherval));
                    assignment.push_back(make_pair(1, otherval));
                    return true;
                }
            }
        }
        return false;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Memory management.
    
    Support* getFreeSupport() {
        // Either get a Support off the free list or make one.
        if(supportFreeList==0) {
            Support* sup=new Support();
            sup->next.resize(vars.size(), 0);
            sup->prev.resize(vars.size(), 0);
            sup->literals=getFreeLitlist();
            return sup;
        }
        else {
            Support* temp=supportFreeList;
            supportFreeList=supportFreeList->next[0];
            return temp;
        }
    }
    
    vector<pair<int,int> >* getFreeLitlist() {
        // Either get a spare literal list off the free list or make one.
        if(litsFreeList.size()==0) {
            vector<pair<int,int> >* lits=new vector<pair<int,int> >();
            return lits;
        }
        else {
            vector<pair<int,int> >* lits=litsFreeList.back();
            litsFreeList.pop_back();
            return lits;
        }
    }
    
    // Old from here.
    /*
  virtual BOOL full_check_unsat()
  { 
    int v_size = vars.size();
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
  }*/
  
  virtual void full_propagate()
  {
      findSupports();
  }
  
  
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
      D_ASSERT(array_size == 4);
      
      if(v[0]==v[1] || v[2]==v[3]) return true;
      return false;
      
    }
    
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> ret;
      ret.reserve(vars.size());
      for(unsigned i = 0; i < vars.size(); ++i)
        ret.push_back(vars[i]);
      return ret;
    }
    
    
};  // end of class


