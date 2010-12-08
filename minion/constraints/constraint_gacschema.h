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

// GAC Schema implementation, adapted from Supports GAC.

// Does it place dynamic triggers for the supports.
#define SupportsGACUseDT true

template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint, Backtrackable
{
    struct Support {
        vector<Support*> prev;   // Size r -- some entries null.
        vector<Support*> next;   
        
        int id;
        
        // Prev and next are indexed by variable. Must be Null if the support does
        // not include that variable. 
        
        vector<pair<int,int> >* literals;
        
        Support(int numvars) : literals(0)
        {
            prev.resize(numvars, 0);
            next.resize(numvars, 0);
            literals=new vector<pair<int, int> >();
        }
        
        // Blank one for use as list header. Must resize next before use.
        Support() : literals(0) {}
        
        virtual ~Support() {
            delete literals;
        }
    };
    
    virtual string constraint_name()
    {
        return "GACSchema";
    }
    
    VarArray vars;
    
    int numvals;
    int dom_min;
    int dom_max;
    
    // 2d array (indexed by var then val) of sentinels,
    // at the head of list of supports. 
    // Needs a sentinel at the start so that dlx-style removals work correctly.
    vector<vector<Support> >  supportListPerLit;
    
    vector<vector<pair<int, int> > > litsPerSupport;  // The structure S from paper.
    // Maps a support (number) to the values that it is the current support for. 
    
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
        
        supportListPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            supportListPerLit[i].resize(numvals);  // blank Support objects.
            for(int j=0; j<numvals; j++) supportListPerLit[i][j].next.resize(vars.size());
        }
        
        
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Dtor
    
    virtual ~ShortSupportsGAC() {
        //printStructures();
        
        // Go through supportFreeList
        for(int var=0; var<vars.size(); var++) {
            for(int val=dom_min; val<=dom_max; val++) {
                Support* sup = supportListPerLit[var][val-dom_min].next[var];
                while(sup!=0) {
                    vector<Support*>& prev=sup->prev;
                    vector<Support*>& next=sup->next;
                    vector<pair<int, int> >& litlist=*(sup->literals);
                    // Unstitch supList from all lists it is in.
                    for(int i=0; i<litlist.size(); i++) {
                        int var=litlist[i].first;
                        D_ASSERT(prev[var]!=0);
                        prev[var]->next[var]=next[var];
                        if(next[var]!=0) {
                            next[var]->prev[var]=prev[var];
                        }
                    }
                    
                    Support* temp=sup;
                    sup=supportListPerLit[var][val-dom_min].next[var];
                    delete temp;
                }
            }
        }
        
        while(supportFreeList!=0) {
            Support* sup=supportFreeList;
            supportFreeList=sup->next[0];
            delete sup;
        }
        
        for(int i=0; i<backtrack_stack.size(); i++) {
            if(backtrack_stack[i].is_removal) {
                delete backtrack_stack[i].sup;
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Backtracking mechanism
    
    struct BTRecord {
        bool is_removal;   // removal or addition was made. 
        Support* sup;
        int var;   // The literal, for when it's a removal. 
        int val;
        
        friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec)
        {
            if(rec.sup==0) return o<<"ZeroMarker";
            o<<"BTRecord:"<<rec.is_removal<<",";
            o<< *((*rec.sup).literals);
            return o;
        }
    };
    
    vector<BTRecord> backtrack_stack;
    
    void mark() {
        struct BTRecord temp = { false, 0, 0, 0 };
        backtrack_stack.push_back(temp);  // marker.
    }
    
    void pop() {
        //cout << "BACKTRACKING:" << endl;
        //cout << backtrack_stack <<endl;
        while(backtrack_stack.back().sup != 0) {
            BTRecord temp=backtrack_stack.back();
            backtrack_stack.pop_back();
            if(temp.is_removal) {
                // The thing was removed from one list, re-insert it into that list.
                addSupportInternal(0, temp.sup, temp.var, temp.val);
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
    
    // For use by the backtracker.
    inline Support* addSupportInternal(Support* sup, int var, int val)
    {
        // Adds sup to the list for var, val only.
        int validx=val-dom_min;
        
        sup->prev[var]= &(supportListPerLit[var][validx]);
        sup->next[var]= supportListPerLit[var][validx].next[var];
        supportListPerLit[var][validx].next[var]=sup;
        if(sup->next[var] != 0)
            sup->next[var]->prev[var]=sup;
    }
    
    
    // Can take either a box or a support object (for use when backtracking). 
    Support* addSupportInternal(box<pair<int, DomainInt> >* litbox, Support* sup)
    {
        // add a new support given as a vector of literals.
        Support* sup_internal;
        
        if(litbox!=0) {
            // copy.
            sup_internal=getFreeSupport();
            sup_internal->literals->clear();
            for(int i=0; i<litbox->size(); i++) sup_internal->literals->push_back((*litbox)[i]);
        }
        else {
            sup_internal=sup;
        }
        vector<pair<int, int> >& litlist_internal=*(sup_internal->literals);
        
        //cout << "Adding support (internal) :" << litlist_internal << endl;
        D_ASSERT(litlist_internal.size()>0);  // It should be possible to deal with empty supports, but currently they wil
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
            
        }
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
        vector<pair<int, int> >& litlist=*(sup->literals);
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
    
    void deleteSupport(Support* sup, int var, int val)
    {
        // Deletes sup from the list for var, val only.
        struct BTRecord temp;
        temp.is_removal=true;
        temp.sup=sup;
        temp.var=var;
        temp.val=val;
        backtrack_stack.push_back(temp);
        
        sup->prev[var]->next[var]=sup->next[var];
        if(sup->next[var]!=0) {
            sup->next[var]->prev[var]=sup->prev[var];
        }
        
    }
    
    
    ////////////////////////////////////////////////////////////////////////////
    // 
    void printStructures()
    {
        cout << "PRINTING ALL DATA STRUCTURES" <<endl;
        
        cout << "Supports for each literal:"<<endl;
        for(int var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(int val=dom_min; val<=dom_max; val++) {
                cout << "Value: "<<val<<endl;
                Support* sup=supportListPerLit[var][val-dom_min].next[var];
                while(sup!=0) {
                    cout << "Support: " << *(sup->literals) << endl;
                    bool contains_varval=false;
                    for(int i=0; i<sup->literals->size(); i++) {
                        if((*(sup->literals))[i].first==var && (*(sup->literals))[i].second==val)
                            contains_varval=true;
                    }
                    D_ASSERT(contains_varval);
                    
                    sup=sup->next[var];
                }
            }
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
    
    int dynamic_trigger_count() { 
        return vars.size()*numvals;
    }
    
  inline void attach_trigger(int var, int val)
  {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+(var*numvals)+(val-dom_min);
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );
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
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Memory management.
    
    Support* getFreeSupport() {
        // Either get a Support off the free list or make one.
        if(supportFreeList==0) {
            Support* sp= new Support(vars.size());
            sp.id=litsPerSupport.size();
            vector<pair<int,int> > temp;
            litsPerSupport.push_back(temp);
        }
        else {
            Support* temp=supportFreeList;
            supportFreeList=supportFreeList->next[0];
            litsPerSupport[temp.id].clear();
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
    
    
    Support* seekInferableSupport
    
};  // end of class


