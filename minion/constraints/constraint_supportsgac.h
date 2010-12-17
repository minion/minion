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

// Does it place dynamic triggers for the supports.
#define SupportsGACUseDT true

// Switches on the zeroVals array. Possibly 
// This flag is a slowdown on qg-supportsgac-7-9
#define SupportsGACUseZeroVals false

template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint, Backtrackable
{
    struct Support {
        vector<Support*> prev;   // Size r -- some entries null.
        vector<Support*> next;   
        
        // Prev and next are indexed by variable. Must be Null if the support does
        // not include that variable. 
        
        vector<pair<int,int> > literals;
        
        Support(int numvars)
        {
            prev.resize(numvars, 0);
            next.resize(numvars, 0);
        }
        
        // Blank one for use as list header. Must resize next before use.
        Support() {}
    };
    
    virtual string constraint_name()
    {
        return "ShortSupportsGAC";
    }
    
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
    #if SupportsGACUseZeroVals
    vector<vector<int> > zeroVals;
    #endif
    
    // Partition of variables by number of supports.
    vector<int> varsPerSupport;    // Permutation of the variables
    vector<int> varsPerSupInv;   // Inverse mapping of the above.
    
    vector<int> supportNumPtrs;   // rd+1 indices into varsPerSupport representing the partition
    
    Support* supportFreeList;       // singly-linked list of spare Support objects.
    
    ////////////////////////////////////////////////////////////////////////////
    // Ctor
    
    ShortSupportsGAC(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj), 
    vars(_var_array), supportFreeList(0)
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
            for(int j=0; j<numvals; j++) supportListPerLit[i][j].next.resize(vars.size());
        }
        
        #if SupportsGACUseZeroVals
        zeroVals.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            zeroVals[i].reserve(numvals);  // reserve the maximum length.
            for(int j=dom_min; j<=dom_max; j++) zeroVals[i].push_back(j);
        }
        #endif
        
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
    // Dtor
    
    virtual ~ShortSupportsGAC() {
        //printStructures();
        set<Support*> myset;
        
        // Go through supportFreeList
        for(int var=0; var<vars.size(); var++) {
            for(int val=dom_min; val<=dom_max; val++) {
                Support* sup = supportListPerLit[var][val-dom_min].next[var];
                while(sup!=0) {
                    vector<Support*>& prev=sup->prev;
                    vector<Support*>& next=sup->next;
                    vector<pair<int, int> >& litlist=sup->literals;
                    // Unstitch supList from all lists it is in.
                    for(int i=0; i<litlist.size(); i++) {
                        int var=litlist[i].first;
                        //D_ASSERT(prev[var]!=0);  // Only for igac. Here it might not be in the list.
                        if(prev[var]!=0) {
                            prev[var]->next[var]=next[var];
                            //prev[var]=0;
                        }
                        if(next[var]!=0) {
                            next[var]->prev[var]=prev[var];
                            //next[var]=0;
                        }
                    }
                    
                    Support* temp=sup;
                    sup=supportListPerLit[var][val-dom_min].next[var];
                    myset.insert(temp);
                }
            }
        }
        
        while(supportFreeList!=0) {
            Support* sup=supportFreeList;
            supportFreeList=sup->next[0];
            myset.insert(sup);
        }
        
        for(int i=0; i<backtrack_stack.size(); i++) {
            if(backtrack_stack[i].sup!=0) {
                myset.insert(backtrack_stack[i].sup);
            }
        }
        
        typename set<Support*>::iterator it;
        for ( it=myset.begin() ; it != myset.end(); it++ ) {
            delete *it;
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Backtracking mechanism
    
    struct BTRecord {
        bool is_removal;   // removal or addition was made. 
        Support* sup;
        
        friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec)
        {
            if(rec.sup==0) return o<<"ZeroMarker";
            o<<"BTRecord:"<<rec.is_removal<<",";
            o<< rec.sup->literals;
            return o;
        }
    };
    
    vector<BTRecord> backtrack_stack;
    
    void mark() {
        struct BTRecord temp = { false, 0 };
        backtrack_stack.push_back(temp);  // marker.
    }
    
    void pop() {
        //cout << "BACKTRACKING:" << endl;
        //cout << backtrack_stack <<endl;
        while(backtrack_stack.back().sup != 0) {
            BTRecord temp=backtrack_stack.back();
            backtrack_stack.pop_back();
            if(temp.is_removal) {
                addSupportInternal(0, temp.sup);
            }
            else {
                deleteSupportInternal(temp.sup, true);
            }
        }
        
        backtrack_stack.pop_back();  // Pop the marker.
        //cout << "END OF BACKTRACKING." << endl;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Add and delete support
    
    Support* addSupport(box<pair<int, DomainInt> >* litlist)
    {
        Support* newsup=addSupportInternal(litlist, 0);
        struct BTRecord temp;
        temp.is_removal=false;
        temp.sup=newsup;
        backtrack_stack.push_back(temp);
        return newsup;
    }
    
    // Can take either a box or a support object (for use when backtracking). 
    Support* addSupportInternal(box<pair<int, DomainInt> >* litbox, Support* sup)
    {
        // add a new support given as a vector of literals.
        Support* sup_internal;
        
        if(litbox!=0) {
            // copy.
            sup_internal=getFreeSupport();
            sup_internal->literals.clear();
            for(int i=0; i<litbox->size(); i++) sup_internal->literals.push_back((*litbox)[i]);
        }
        else {
            sup_internal=sup;
        }
        vector<pair<int, int> >& litlist_internal=sup_internal->literals;
        
        //cout << "Adding support (internal) :" << litlist_internal << endl;
        //D_ASSERT(litlist_internal.size()>0);  // It should be possible to deal with empty supports, but currently they wil
        // cause a memory leak. 
        
        int litsize=litlist_internal.size();
        for(int i=0; i<litsize; i++) {
            pair<int, int> temp=litlist_internal[i];
            int var=temp.first;
            int val=temp.second-dom_min;
            
            // Stitch it into supportListPerLit
            sup_internal->prev[var]= &(supportListPerLit[var][val]);
            sup_internal->next[var]= supportListPerLit[var][val].next[var];
            supportListPerLit[var][val].next[var]=sup_internal;
            if(sup_internal->next[var] != 0)
                sup_internal->next[var]->prev[var]=sup_internal;
            
            //update counters
            supportsPerVar[var]++;
            supportsPerLit[var][val]++;
            
            // Attach trigger if this is the first support containing var,val.
            if(SupportsGACUseDT && supportsPerLit[var][val]==1) {
                attach_trigger(var, val+dom_min);
            }
            
            // Update partition
            // swap var to the end of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]]-1]);
            // Move the boundary so var is now in the higher cell.
            supportNumPtrs[supportsPerVar[var]]--;
        }
        supports++;
        
        //printStructures();
        
        return sup_internal;
    }
    
    void deleteSupport(Support* sup) {
        struct BTRecord temp;
        temp.is_removal=true;
        temp.sup=sup;
        backtrack_stack.push_back(temp);
        
        deleteSupportInternal(sup, false);
    }
    
    void deleteSupportInternal(Support* sup, bool Backtracking) {
        D_ASSERT(sup!=0);
        
        // Remove sup from supportListPerLit
        vector<Support*>& prev=sup->prev;
        vector<Support*>& next=sup->next;
        vector<pair<int, int> >& litlist=sup->literals;
        //cout << "Removing support (internal) :" << litlist << endl;
        
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
            D_ASSERT(supportsPerLit[var][litlist[i].second-dom_min] >= 0);
            
            #if SupportsGACUseZeroVals
            if(supportsPerLit[var][litlist[i].second-dom_min]==0) {
                zeroVals[var].push_back(litlist[i].second);
            }
            #endif
            
            // Remove trigger if this is the last support containing var,val.
            if(SupportsGACUseDT && supportsPerLit[var][litlist[i].second-dom_min]==0) {
                detach_trigger(var, litlist[i].second);
            }
            
            // Update partition
            // swap var to the start of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]+1]]);
            // Move the boundary so var is now in the lower cell.
            supportNumPtrs[supportsPerVar[var]+1]++;
        }
        supports--;
        
        //printStructures();
        
        if(Backtracking) {
            // Can re-use the support when it is removed by BT. 
            // Stick it on the free list using next[0] as the next ptr.
            sup->next[0]=supportFreeList;
            supportFreeList=sup;
        }
        // else can't re-use it because a ptr to it is on the BT stack. 
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // 
    void printStructures()
    {
        cout << "PRINTING ALL DATA STRUCTURES" <<endl;
        cout << "supports:" << supports <<endl;
        cout << "supportsPerVar:" << supportsPerVar << endl;
        cout << "supportsPerLit:" << supportsPerLit << endl;
        cout << "partition:" <<endl;
        for(int i=0; i<supportNumPtrs.size()-1; i++) {
            cout << "supports: "<< i<< "  vars: ";
            for(int j=supportNumPtrs[i]; j<supportNumPtrs[i+1]; j++) {
                cout << varsPerSupport[j]<< ", ";
            }
            cout << endl;
            if(supportNumPtrs[i+1]==vars.size()) break;
        }
        #if SupportsGACUseZeroVals
        cout << "zeroVals:" << zeroVals << endl;
        #endif
        
        cout << "Supports for each literal:"<<endl;
        for(int var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(int val=dom_min; val<=dom_max; val++) {
                cout << "Value: "<<val<<endl;
                Support* sup=supportListPerLit[var][val-dom_min].next[var];
                while(sup!=0) {
                    cout << "Support: " << sup->literals << endl;
                    bool contains_varval=false;
                    for(int i=0; i<sup->literals.size(); i++) {
                        if(sup->literals[i].first==var && sup->literals[i].second==val)
                            contains_varval=true;
                    }
                    D_ASSERT(contains_varval);
                    
                    sup=sup->next[var];
                }
            }
        }
    }
    
    #if !SupportsGACUseDT
        virtual triggerCollection setup_internal()
        {
            triggerCollection t;
            int array_size = vars.size();
            for(int i = 0; i < array_size; ++i)
              t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
            return t;
        }
    #endif
    
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
    
    void findSupports()
    {
        // For each variable where the number of supports is equal to the total...
    restartloop:
        for(int i=supportNumPtrs[supports]; i<supportNumPtrs[supports+1]; i++) {
            int var=varsPerSupport[i];
            
            #if !SupportsGACUseZeroVals
            for(int val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
            #else
            for(int j=0; j<zeroVals[var].size(); j++) {
                int val=zeroVals[var][j];
                if(supportsPerLit[var][val-dom_min]>0) {
                    // No longer a zero val. remove from vector.
                    zeroVals[var][j]=zeroVals[var][zeroVals[var].size()-1];
                    zeroVals[var].pop_back();
                    j--;
                    continue;
                }
            #endif
                
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
                        
                        #if SupportsGACUseZeroVals
                        if(supportsPerLit[var][val-dom_min]>0) {
                            // No longer a zero val. remove from vector.
                            zeroVals[var][j]=zeroVals[var][zeroVals[var].size()-1];
                            zeroVals[var].pop_back();
                        }
                        #endif
                        
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
    
    inline void updateCounters(int var, int val) {
        Support* supList = supportListPerLit[var][val-dom_min].next[var];
        while(supList != 0) {
            Support* next=supList->next[var];
            deleteSupport(supList);
            supList=next;
        }
    }
    
    
    #if SupportsGACUseDT
        int dynamic_trigger_count() { 
            return vars.size()*numvals;
        }
    #endif
    
  inline void attach_trigger(int var, int val)
  {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+(var*numvals)+(val-dom_min);
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  inline void detach_trigger(int var, int val)
  {
      //P("Detach Triggers");
      
      D_ASSERT(supportsPerLit[var][val-dom_min] == 0);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      dt=dt+(var*numvals)+(val-dom_min);
      releaseTrigger(stateObj, dt );   // BT_CALL_BACKTRACK
  }
    
  virtual void propagate(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<vars.size());
    // Really needs triggers on each value, or on the supports. 
    
    //printStructures();
    D_ASSERT(!SupportsGACUseDT);  // Should not be here if using dynamic triggers.
    
    for(int val=dom_min; val<=dom_max; val++) {
        if(!vars[prop_var].inDomain(val) && supportsPerLit[prop_var][val-dom_min]>0) {
            updateCounters(prop_var, val);
        }
    }
    
    findSupports();
  }
  
    virtual void propagate(DynamicTrigger* dt)
  {
      int pos=dt-dynamic_trigger_start();
      int var=pos/numvals;
      int val=pos-(var*numvals)+dom_min;
      
      updateCounters(var, val);
      
      findSupports();
  }
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for pair-equals. a=b or c=d.
    
    /*
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
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
      D_ASSERT(array_size == 4);
      
      if(v[0]==v[1] || v[2]==v[3]) return true;
      return false;
      
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for element
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        typedef typename VarArray::value_type VarRef;
        VarRef idxvar=vars[vars.size()-2];
        VarRef resultvar=vars[vars.size()-1];
        D_ASSERT(vars[var].inDomain(val));
        
        if(var<vars.size()-2) {
            // var is in the vector.
            
            for(int i=idxvar.getMin(); i<=idxvar.getMax(); i++) {
                if(idxvar.inDomain(i) && i>=0 && i<vars.size()-2) {
                    for(int j=resultvar.getMin(); j<=resultvar.getMax(); j++) {
                        if(resultvar.inDomain(j) && vars[i].inDomain(j) &&
                            (i!=var || j==val) ) {   // Either the support includes both var, val or neither -- if neither, it will be a support for var,val.
                            assignment.push_back(make_pair(i, j));
                            assignment.push_back(make_pair(vars.size()-2, i));
                            assignment.push_back(make_pair(vars.size()-1, j));
                            return true;
                        }
                    }
                }
            }
        }
        else if(var==vars.size()-2) {
            // It's the index variable.
            if(val<0 || val>=vars.size()-2){
                return false;
            }
            
            for(int i=resultvar.getMin(); i<=resultvar.getMax(); i++) {
                if(resultvar.inDomain(i) && vars[val].inDomain(i)) {
                    assignment.push_back(make_pair(vars.size()-2, val));
                    assignment.push_back(make_pair(vars.size()-1, i));
                    assignment.push_back(make_pair(val, i));
                    return true;
                }
            }
            
        }
        else if(var==vars.size()-1) {
            // The result variable.
            for(int i=0; i<vars.size()-2; i++) {
                if(vars[i].inDomain(val) && idxvar.inDomain(i)) {
                    assignment.push_back(make_pair(vars.size()-2, i));
                    assignment.push_back(make_pair(vars.size()-1, val));
                    assignment.push_back(make_pair(i, val));
                    return true;
                }
            }
        }
        return false;
        
        
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        int idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for lexleq
    
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()%2==0);
        // First part of vars is vector 1.
        int vecsize=vars.size()/2;
        
        for(int i=0; i<vecsize; i++) {
            int j=i+vecsize;
            int jmax=vars[j].getMax();
            int imin=vars[i].getMin();
            
            // CASE 1   It is not possible for the pair to be equal or less.
            if(imin>jmax) {
                return false;
            }
            
            // CASE 2    It is only possible to make the pair equal.
            if(imin==jmax) {
                // check against var, val here.
                if(i==var && imin!=val) {
                    return false;
                }
                if(j==var && jmax!=val) {
                    return false;
                }
                
                if(!vars[i].isAssigned()) {
                    assignment.push_back(make_pair(i, imin));    
                }
                if(!vars[j].isAssigned()) {
                    assignment.push_back(make_pair(j, jmax));
                }
                // Do not return, continue along the vector.
                continue;
            }
            
            // CASE 3    It is possible make the pair less.
            if(imin<jmax) {
                if(i==var) {
                    if(val==jmax) {
                        assignment.push_back(make_pair(i,val));
                        assignment.push_back(make_pair(j,val));
                        continue;
                    }
                    else if(val>jmax) {
                        return false;
                    }
                    else {   //  val<jmax
                        assignment.push_back(make_pair(var, val));
                        assignment.push_back(make_pair(j, jmax));
                        return true;
                    }
                }
                
                if(j==var) {
                    if(val==imin) {
                        assignment.push_back(make_pair(i,val));
                        assignment.push_back(make_pair(j,val));
                        continue;
                    }
                    else if(val<imin) {
                        return false;
                    }
                    else {   //  val>imin
                        assignment.push_back(make_pair(var, val));
                        assignment.push_back(make_pair(i, imin));
                        return true;
                    }
                }
                
                
                // BETTER NOT TO USE min and max here, should watch something in the middle of the domain...
                
                
                assignment.push_back(make_pair(i, imin));
                assignment.push_back(make_pair(j, jmax));
                return true;
            }
            
        }
        
        // Got to end of vector without finding a pair that can satisfy
        // the ct. However this is equal....
        return true;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        D_ASSERT(array_size%2==0);
        for(int i=0; i<array_size/2; i++)
        {
            if(v[i]<v[i+array_size/2]) return true;
            if(v[i]>v[i+array_size/2]) return false;
        }
        return true;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Memory management.
    
    Support* getFreeSupport() {
        // Either get a Support off the free list or make one.
        if(supportFreeList==0) {
            return new Support(vars.size());
        }
        else {
            Support* temp=supportFreeList;
            supportFreeList=supportFreeList->next[0];
            return temp;
        }
    }
    
    virtual void full_propagate()
    {
        findSupports();
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


