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
//#define SupportsGACUseDT true

template<typename VarArray>
struct GACSchema : public AbstractConstraint, Backtrackable
{
    struct Support {
        vector<Support*> prev;   // Size r -- some entries null.
        vector<Support*> next;   
        
        int id;
        
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
        
        friend std::ostream& operator<<(std::ostream& o, const Support& sp)
        {
            o<<"Support: "<<sp.id<<" "<<sp.literals;
            return o;
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
    
    GACSchema(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj), 
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
    
    virtual ~GACSchema() {
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
            if(backtrack_stack[i].typ==1) {
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
        unsigned char typ;
        // typ is the type of event that happened:
        // 0: addition (to all lists), 
        // 1: removal (from one list), 
        // 2: addition of lit to litsPerSupport
        // 3: removal of lit from litsPerSupport
        
        Support* sup;
        int var;   // The literal, for when it's a removal. 
        int val;
        
        friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec)
        {
            if(rec.sup==0) return o<<"ZeroMarker";
            o<<"BTRecord:"<<rec.typ<<",";
            o<< rec.sup->literals;
            return o;
        }
    };
    
    vector<BTRecord> backtrack_stack;
    
    void mark() {
        struct BTRecord temp = { 0, 0, 0, 0 };
        backtrack_stack.push_back(temp);  // marker.
    }
    
    void pop() {
        //cout << "BACKTRACKING:" << endl;
        //cout << backtrack_stack <<endl;
        while(backtrack_stack.back().sup != 0) {
            BTRecord temp=backtrack_stack.back();
            backtrack_stack.pop_back();
            if(temp.typ==1) {
                // The thing was removed from one list, re-insert it into that list.
                addSupportInternal(temp.sup, temp.var, temp.val);
            }
            else if (temp.typ==0) {
                deleteSupportInternal(temp.sup, true);
            }
            else if(temp.typ==2) {
                // var,val was added to litsPerSupport for sup.
                int id=temp.sup->id;
                //D_ASSERT(litsPerSupport[id].back().first==temp.var && litsPerSupport[id].back().second==temp.val);
                if(!(litsPerSupport[id].back().first==temp.var && litsPerSupport[id].back().second==temp.val)) {
                    cout << "Can't pop pair "<< temp.var<<","<<temp.val << "from litsPerSupport "<< id <<" "<<litsPerSupport[id]<<endl;
                    abort();
                }
                litsPerSupport[id].pop_back();
            }
            else if(temp.typ==3) {
                int id=temp.sup->id;
                litsPerSupport[id].push_back(make_pair(temp.var, temp.val));
            }
        }
        
        backtrack_stack.pop_back();  // Pop the marker.
        //cout << "END OF BACKTRACKING." << endl;
    }
    
    
    inline void addToLitsPerSupport(Support* sp, int var, int val) {
        litsPerSupport[sp->id].push_back(make_pair(var,val));
        // Add bt record
        struct BTRecord temp = { 2, sp, var, val };
        backtrack_stack.push_back(temp);
    }
    
    inline void deleteFromLitsPerSupport(Support* sp, int var, int val) {
        D_ASSERT(litsPerSupport[sp->id].back().first == var && litsPerSupport[sp->id].back().second ==val);
        litsPerSupport[sp->id].pop_back();
        // Add bt record
        struct BTRecord temp = { 3, sp, var, val };
        backtrack_stack.push_back(temp);
    }
    
    
    ////////////////////////////////////////////////////////////////////////////
    // Add and delete support
    
    Support* addSupport(box<pair<int, DomainInt> >* litlist)
    {
        Support* newsup=addSupportInternal(litlist, 0);
        struct BTRecord temp;
        temp.typ=0;
        temp.sup=newsup;
        backtrack_stack.push_back(temp);
        return newsup;
    }
    
    // For use by the backtracker.
    inline void addSupportInternal(Support* sup, int var, int val)
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
            sup_internal->literals.clear();
            for(int i=0; i<litbox->size(); i++) {sup_internal->literals.push_back((*litbox)[i]); }
        }
        else {
            sup_internal=sup;
        }
        vector<pair<int, int> >& litlist_internal=sup_internal->literals;
        
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
        vector<pair<int, int> >& litlist=sup->literals;
        //cout << "Removing support (internal) :" << litlist << endl;
        
        for(int i=0; i<litlist.size(); i++) {
            int var=litlist[i].first;
            D_ASSERT(prev[var]!=0);
            prev[var]->next[var]=next[var];
            if(next[var]!=0) {
                next[var]->prev[var]=prev[var];
            }
            prev[var]=0;
            next[var]=0;
        }
        
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
        //cout << "DeleteSupport " << *sup << " var,val : "<< var <<","<< val <<endl;
        struct BTRecord temp;
        temp.typ=1;
        temp.sup=sup;
        temp.var=var;
        temp.val=val;
        backtrack_stack.push_back(temp);
        
        vector<Support*>& prev=sup->prev;
        vector<Support*>& next=sup->next;
        
        prev[var]->next[var]=next[var];
        if(next[var]!=0) {
            next[var]->prev[var]=prev[var];
        }
        prev[var]=0;
        next[var]=0;
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
                if(vars[var].inDomain(val)) {
                    cout << "Value: "<<val<<endl;
                    Support* sup=supportListPerLit[var][val-dom_min].next[var];
                    while(sup!=0) {
                        cout << *(sup) << endl;
                        bool contains_varval=false;
                        for(int i=0; i<sup->literals.size(); i++) {
                            if(sup->literals[i].first==var && sup->literals[i].second==val)
                                contains_varval=true;
                        }
                        D_ASSERT(contains_varval);
                        
                        D_ASSERT(sup->next[var]==0 || sup->next[var]->prev[var] == sup);
                        sup=sup->next[var];
                    }
                }
            }
        }
        
        cout << "Literals for each Support:"<< endl;
        for(int i=0; i<litsPerSupport.size(); i++) {
            cout << i<< ", " << litsPerSupport[i] << endl;
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
      int validx=val-dom_min;
      
      //cout << "Entered propagate."<<endl;
      //printStructures();
      
      while(supportListPerLit[var][validx].next[var]!=0) {
          Support* tau = supportListPerLit[var][validx].next[var];
          //cout << "In main loop for support: "<< *(tau) << ", " << "var:"<< var << " val:"<<val <<endl;
          
          D_ASSERT(tau->prev[var]==&(supportListPerLit[var][validx]) );
          // Delete tau from all lists it is in.  NOT THE SAME AS THE PAPER, WHICH HAS A BUG AT THIS POINT.
          for(int i=0; i<vars.size(); i++) {
              pair<int,int> lit=tau->literals[i];
              if(tau->prev[lit.first] != 0) {   // If in list it has a prev ptr.
                  deleteSupport(tau, lit.first, lit.second);
              }
          }
          //printStructures();
          D_ASSERT(supportListPerLit[var][validx].next[var]!=tau);
          
          while(litsPerSupport[tau->id].size()>0) {
              pair<int, int> lit=litsPerSupport[tau->id].back();
              deleteFromLitsPerSupport(tau, lit.first, lit.second);
              
              if(vars[lit.first].inDomain(lit.second)) {
                  Support* sigma=seekInferableSupport(lit.first, lit.second);
                  if(sigma!=0) {
                      addToLitsPerSupport(sigma, lit.first, lit.second);
                  }
                  else {
                      typedef pair<int,DomainInt> temptype;
                      MAKE_STACK_BOX(newsupportbox, temptype, vars.size()); 
                      bool foundsupport=findNewSupport(newsupportbox, lit.first, lit.second);
                      if(foundsupport) {
                          Support* sp=addSupport(&newsupportbox);
                          addToLitsPerSupport(sp, lit.first, lit.second);
                      }
                      else {
                          vars[lit.first].removeFromDomain(lit.second);
                      }
                  }
              }
          }
      }
  }
    
    // Macro to add either the lower bound or the specified value for a particular variable vartopad
    // Intended to pad out an assignment to a full-length support.
    #define PADOUT(vartopad) if(var==vartopad) assignment.push_back(make_pair(var, val)); else assignment.push_back(make_pair(vartopad, vars[vartopad].getMin()));
    
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
    // Changed compared to supportsgac version to return full tuple.
    
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
                            for(int k=0; k<vars.size()-2; k++) {
                                if(k!=i) {
                                    if(k==var)
                                        assignment.push_back(make_pair(k, val));
                                    else
                                        assignment.push_back(make_pair(k, vars[k].getMin()));
                                }
                            }
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
                    for(int k=0; k<vars.size()-2; k++) {
                        if(k!=val) assignment.push_back(make_pair(k, vars[k].getMin()));
                    }
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
                    for(int k=0; k<vars.size()-2; k++) {
                        if(k!=i) assignment.push_back(make_pair(k, vars[k].getMin()));
                    }
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
    */
    
    ////////////////////////////////////////////////////////////////////////////
    // 
    // Square packing
    
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        int i=vars[4].getAssignedValue();
        int j=vars[5].getAssignedValue();
        
        // object i below object j.
        if(vars[1].getMin()+i <=vars[3].getMax()) {
            if(var==1) {
                if(val+i<=vars[3].getMax()) { 
                    assignment.push_back(make_pair(1, val));
                    assignment.push_back(make_pair(3, vars[3].getMax()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(vars[1].getMin()+i<=val) {
                    assignment.push_back(make_pair(1, vars[1].getMin()));
                    assignment.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(1, vars[1].getMin()));
                assignment.push_back(make_pair(3, vars[3].getMax()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i above object j
        if(vars[3].getMin()+j <= vars[1].getMax()) {
            if(var==1) {
                if(vars[3].getMin()+j <= val) {
                    assignment.push_back(make_pair(1, val));
                    assignment.push_back(make_pair(3, vars[3].getMin()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(val+j <= vars[1].getMax())
                {
                    assignment.push_back(make_pair(1, vars[1].getMax()));
                    assignment.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(1, vars[1].getMax()));
                assignment.push_back(make_pair(3, vars[3].getMin()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i left of object j.
        if(vars[0].getMin()+i <=vars[2].getMax()) {
            if(var==0) {
                if(val+i <=vars[2].getMax()) {
                    assignment.push_back(make_pair(0, val));
                    assignment.push_back(make_pair(2, vars[2].getMax()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(vars[0].getMin()+i <=val) {
                    assignment.push_back(make_pair(0, vars[0].getMin()));
                    assignment.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(0, vars[0].getMin()));
                assignment.push_back(make_pair(2, vars[2].getMax()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        // object i right of object j
        if(vars[2].getMin()+j <= vars[0].getMax()) {
            if(var==0) {
                if(vars[2].getMin()+j <= val) {
                    assignment.push_back(make_pair(0, val));
                    assignment.push_back(make_pair(2, vars[2].getMin()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(val+j <= vars[0].getMax()) {
                    assignment.push_back(make_pair(0, vars[0].getMax()));
                    assignment.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(0, vars[0].getMax()));
                assignment.push_back(make_pair(2, vars[2].getMin()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        // argh, how to do this.
        // test with element first
        
        // object i above object j.
        if(v[1]+v[4] <=v[3]) {
            return true;
        }
        
        // object i below object j
        if(v[3]+v[5] <= v[1]) {
            return true;
        }
        
        // object i left of object j.
        if(v[0]+v[4] <=v[2]) {
            return true;
        }
        
        // object i right of object j
        if(v[2]+v[5] <= v[0]) {
            return true;
        }
        return false;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Memory management.
    
    Support* getFreeSupport() {
        // Either get a Support off the free list or make one.
        if(supportFreeList==0) {
            Support* sp= new Support(vars.size());
            sp->id=litsPerSupport.size();
            vector<pair<int,int> > temp;
            litsPerSupport.push_back(temp);
            return sp;
        }
        else {
            Support* temp=supportFreeList;
            supportFreeList=supportFreeList->next[0];
            D_ASSERT(litsPerSupport[temp->id].size()==0);   // Should be emptied by backtracking.
            //litsPerSupport[temp.id].clear();
            return temp;
        }
    }
    
    virtual void full_propagate()
    {
        D_ASSERT(backtrack_stack.size()==0);
        // For each literal, find a support for it or delete it. 
        for(int var=0; var<vars.size(); var++) {
            for(int val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
                if(vars[var].inDomain(val)) {
                    
                    // From here is cut-and-paste from propagate.
                    Support* sigma=seekInferableSupport(var, val);
                      if(sigma!=0) {
                          addToLitsPerSupport(sigma, var, val);
                      }
                      else {
                          typedef pair<int,DomainInt> temptype;
                          MAKE_STACK_BOX(newsupportbox, temptype, vars.size());
                          bool foundsupport=findNewSupport(newsupportbox, var, val);
                          if(foundsupport) {
                              Support* sp=addSupport(&newsupportbox);
                              addToLitsPerSupport(sp, var,val);
                          }
                          else {
                              vars[var].removeFromDomain(val);
                          }
                      }
                }
                
                if(vars[var].inDomain(val)) {
                    // If the value is still there, Put trigger on.
                    attach_trigger(var,val);
                    D_ASSERT(supportListPerLit[var][val-dom_min].next[var]!=0);
                }
            }
        }
    }
    
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> ret;
      ret.reserve(vars.size());
      for(unsigned i = 0; i < vars.size(); ++i)
        ret.push_back(vars[i]);
      return ret;
    }
    
    
    Support* seekInferableSupport(int var, int val) {
        int validx=val-dom_min;
        while(supportListPerLit[var][validx].next[var]!=0) {
            Support* temp=supportListPerLit[var][validx].next[var];
            vector<pair<int, int> >& literals=temp->literals;
            bool lost=false;
            for(int i=0; i<vars.size(); i++) {
                pair<int, int> lit=literals[i];
                if(!vars[lit.first].inDomain(lit.second)) {
                    lost=true; break;
                }
            }
            if(lost) {
                deleteSupport(temp, var, val);
            }
            else {
                return temp;
            }
        }
        return 0;
    }
    
};  // end of class


