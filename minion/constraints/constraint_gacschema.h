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

#include "../constraints/constraint_checkassign.h"

template<typename VarArray>
struct GACSchema : public AbstractConstraint, Backtrackable
{
    virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    const SysInt tuple_size = checked_cast<SysInt>(data->tuple_size());
    const SysInt length = checked_cast<SysInt>(data->size());
    DomainInt* tuple_data = data->getPointer();

    for(SysInt i = 0; i < length; ++i)
    {
      DomainInt* tuple_start = tuple_data + i*tuple_size;
      bool success = true;
      for(SysInt j = 0; j < tuple_size && success; ++j)
      {
        if(!vars[j].inDomain(tuple_start[j]))
          success = false;
      }
      if(success)
      {
        for(SysInt i = 0; i < tuple_size; ++i)
          assignment.push_back(make_pair(i, tuple_start[i]));
        return true;
      }
    }
    return false;
  }

    virtual AbstractConstraint* reverse_constraint()
    { return forward_check_negation(stateObj, this); }


    struct Support {
        vector<Support*> prev;   // Size r -- some entries null.
        vector<Support*> next;   
        
        SysInt id;
        
        // Prev and next are indexed by variable. Must be Null if the support does
        // not include that variable. 
        
        vector<pair<SysInt,DomainInt> > literals;
        
        Support(SysInt numvars)
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
    
    SysInt numvals;
    DomainInt dom_min;
    DomainInt dom_max;
    
    // 2d array (indexed by var then val) of sentinels,
    // at the head of list of supports. 
    // Needs a sentinel at the start so that dlx-style removals work correctly.
    vector<vector<Support> >  supportListPerLit;
    
    vector<vector<pair<SysInt, DomainInt> > > litsPerSupport;  // The structure S from paper.
    // Maps a support (number) to the values that it is the current support for. 
    
    Support* supportFreeList;       // singly-linked list of spare Support objects.

    // Stuff to do with tuples.
    TupleList* data;
    vector<vector<vector<vector<DomainInt> * > > > tuple_lists;
    vector<MoveableArray<unsigned int> > tuple_list_pos;  // indexed by var, val.

    ////////////////////////////////////////////////////////////////////////////
    // Ctor
    
    GACSchema(StateObj* _stateObj, const VarArray& _var_array, TupleList* _data) : AbstractConstraint(_stateObj), 
    vars(_var_array), supportFreeList(0), data(_data)
    {
        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        if(vars.size()>0) {
            dom_max=vars[0].getInitialMax();
            dom_min=vars[0].getInitialMin();
            for(SysInt i=1; i<vars.size(); i++) {
                if(vars[i].getInitialMin()<dom_min) dom_min=vars[i].getInitialMin();
                if(vars[i].getInitialMax()>dom_max) dom_max=vars[i].getInitialMax();
            }
            numvals=checked_cast<SysInt>(dom_max-dom_min+1);
        }
        
        supportListPerLit.resize(vars.size());
        for(SysInt i=0; i<vars.size(); i++) {
            supportListPerLit[i].resize(numvals);  // blank Support objects.
            for(SysInt j=0; j<numvals; j++) supportListPerLit[i][j].next.resize(vars.size());
        }
         
        // List-specific things
        tuple_list_pos.resize(vars.size());
        
        // populate tuple_lists
        tuple_lists.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            const SysInt domsize = checked_cast<SysInt>(vars[var].getInitialMax()-vars[var].getInitialMin()+1);
            tuple_list_pos[var]=getMemory(_stateObj).backTrack().template requestArray<unsigned int>(domsize);
            for(SysInt validx=0; validx<domsize; validx++) {
                tuple_list_pos[var][validx]=0;
            }
            tuple_lists[var].resize(domsize);
        }
         
        DomainInt numtuples=data->size();
        DomainInt* tupdata=data->getPointer();
        for(DomainInt i=0; i<numtuples; i++) {
            vector<DomainInt>* tup=new vector<DomainInt>(tupdata, tupdata+vars.size() );
            
            for(SysInt var=0; var<vars.size(); var++) {
                DomainInt val=(*tup)[var];
                if(val>= vars[var].getInitialMin() && val<= vars[var].getInitialMax()) {
                    tuple_lists[var][checked_cast<SysInt>(val-vars[var].getInitialMin())].push_back(tup);
                }
            }
            
            tupdata+=vars.size();
        }
     }

    
    ////////////////////////////////////////////////////////////////////////////
    // Dtor
    
    virtual ~GACSchema() {
        //printStructures();
        set<Support*> myset;
        
        // Go through supportFreeList
        for(SysInt var=0; var<vars.size(); var++) {
            for(DomainInt val=dom_min; val<=dom_max; val++) {
                Support* sup = supportListPerLit[var][checked_cast<SysInt>(val-dom_min)].next[var];
                while(sup!=0) {
                    vector<Support*>& prev=sup->prev;
                    vector<Support*>& next=sup->next;
                    vector<pair<SysInt, DomainInt> >& litlist=sup->literals;
                    // Unstitch supList from all lists it is in.
                    for(SysInt i=0; i<litlist.size(); i++) {
                        SysInt var=litlist[i].first;
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
                    sup=supportListPerLit[var][checked_cast<SysInt>(val-dom_min)].next[var];
                    myset.insert(temp);
                }
            }
        }
        
        while(supportFreeList!=0) {
            Support* sup=supportFreeList;
            supportFreeList=sup->next[0];
            myset.insert(sup);
        }
        
        for(SysInt i=0; i<backtrack_stack.size(); i++) {
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
        SysInt var;   // The literal, for when it's a removal. 
        DomainInt val;
        
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
                SysInt id=temp.sup->id;
                //D_ASSERT(litsPerSupport[id].back().first==temp.var && litsPerSupport[id].back().second==temp.val);
                if(!(litsPerSupport[id].back().first==temp.var && litsPerSupport[id].back().second==temp.val)) {
                    cout << "Can't pop pair "<< temp.var<<","<<temp.val << "from litsPerSupport "<< id <<" "<<litsPerSupport[id]<<endl;
                    abort();
                }
                litsPerSupport[id].pop_back();
            }
            else if(temp.typ==3) {
                SysInt id=temp.sup->id;
                litsPerSupport[id].push_back(make_pair(temp.var, temp.val));
            }
        }
        
        backtrack_stack.pop_back();  // Pop the marker.
        //cout << "END OF BACKTRACKING." << endl;
    }
    
    
    inline void addToLitsPerSupport(Support* sp, SysInt var, DomainInt val) {
        litsPerSupport[sp->id].push_back(make_pair(var,val));
        // Add bt record
        struct BTRecord temp = { 2, sp, var, val };
        backtrack_stack.push_back(temp);
    }
    
    inline void deleteFromLitsPerSupport(Support* sp, SysInt var, DomainInt val) {
        D_ASSERT(litsPerSupport[sp->id].back().first == var && litsPerSupport[sp->id].back().second ==val);
        litsPerSupport[sp->id].pop_back();
        // Add bt record
        struct BTRecord temp = { 3, sp, var, val };
        backtrack_stack.push_back(temp);
    }
    
    
    ////////////////////////////////////////////////////////////////////////////
    // Add and delete support
    
    Support* addSupport(box<pair<SysInt, DomainInt> >* litlist)
    {
        D_ASSERT(litlist->size()==vars.size());
        Support* newsup=addSupportInternal(litlist, 0);
        struct BTRecord temp;
        temp.typ=0;
        temp.sup=newsup;
        backtrack_stack.push_back(temp);
        return newsup;
    }
    
    // For use by the backtracker.
    inline void addSupportInternal(Support* sup, SysInt var, DomainInt val)
    {
        // Adds sup to the list for var, val only.
        const SysInt validx=checked_cast<SysInt>(val-dom_min);
        
        sup->prev[var]= &(supportListPerLit[var][validx]);
        sup->next[var]= supportListPerLit[var][validx].next[var];
        supportListPerLit[var][validx].next[var]=sup;
        if(sup->next[var] != 0)
            sup->next[var]->prev[var]=sup;
    }
    
    
    // Can take either a box or a support object (for use when backtracking). 
    Support* addSupportInternal(box<pair<SysInt, DomainInt> >* litbox, Support* sup)
    {
        // add a new support given as a vector of literals.
        Support* sup_internal;
        
        if(litbox!=0) {
            // copy.
            sup_internal=getFreeSupport();
            sup_internal->literals.clear();
            for(SysInt i=0; i<litbox->size(); i++) {sup_internal->literals.push_back((*litbox)[i]); }
        }
        else {
            sup_internal=sup;
        }
        vector<pair<SysInt, DomainInt> >& litlist_internal=sup_internal->literals;
        
        //cout << "Adding support (internal) :" << litlist_internal << endl;
        D_ASSERT(litlist_internal.size()>0);  // It should be possible to deal with empty supports, but currently they wil
        // cause a memory leak. 
        
        SysInt litsize=litlist_internal.size();
        for(SysInt i=0; i<litsize; i++) {
            pair<SysInt, DomainInt> temp=litlist_internal[i];
            SysInt var=temp.first;
            const SysInt val=checked_cast<SysInt>(temp.second-dom_min);
            
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
        vector<pair<SysInt, DomainInt> >& litlist=sup->literals;
        //cout << "Removing support (internal) :" << litlist << endl;
        
        for(SysInt i=0; i<litlist.size(); i++) {
            SysInt var=litlist[i].first;
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
    
    void deleteSupport(Support* sup, SysInt var, DomainInt val)
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
        for(SysInt var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(DomainInt val=dom_min; val<=dom_max; val++) {
                if(vars[var].inDomain(val)) {
                    cout << "Value: "<<val<<endl;
                    Support* sup=supportListPerLit[var][checked_cast<SysInt>(val-dom_min)].next[var];
                    while(sup!=0) {
                        cout << *(sup) << endl;
                        bool contains_varval=false;
                        for(SysInt i=0; i<sup->literals.size(); i++) {
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
        for(SysInt i=0; i<litsPerSupport.size(); i++) {
            cout << i<< ", " << litsPerSupport[i] << endl;
        }
    }
    
    SysInt dynamic_trigger_count() { 
        return vars.size()*numvals;
    }
    
  inline void attach_trigger(SysInt var, DomainInt val)
  {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+checked_cast<SysInt>((var*numvals)+(val-dom_min));
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );
  }
  
  
  virtual void propagate(DynamicTrigger* dt)
  {
      const SysInt pos=dt-dynamic_trigger_start();
      const SysInt var=pos/numvals;
      DomainInt val=pos-(var*numvals)+dom_min;
      const SysInt validx= checked_cast<SysInt>(val-dom_min);
      
      //cout << "Entered propagate."<<endl;
      //printStructures();
      
      while(supportListPerLit[var][validx].next[var]!=0) {
          Support* tau = supportListPerLit[var][validx].next[var];
          //cout << "In main loop for support: "<< *(tau) << ", " << "var:"<< var << " val:"<<val <<endl;
          
          D_ASSERT(tau->prev[var]==&(supportListPerLit[var][validx]) );
          // Delete tau from all lists it is in.  NOT THE SAME AS THE PAPER, WHICH HAS A BUG AT THIS POINT.
          for(SysInt i=0; i<vars.size(); i++) {
              pair<SysInt,DomainInt> lit=tau->literals[i];
              if(tau->prev[lit.first] != 0) {   // If in list it has a prev ptr.
                  deleteSupport(tau, lit.first, lit.second);
              }
          }
          //printStructures();
          D_ASSERT(supportListPerLit[var][validx].next[var]!=tau);
          
          while(litsPerSupport[tau->id].size()>0) {
              pair<SysInt, DomainInt> lit=litsPerSupport[tau->id].back();
              deleteFromLitsPerSupport(tau, lit.first, lit.second);
              
              if(vars[lit.first].inDomain(lit.second)) {
                  Support* sigma=seekInferableSupport(lit.first, lit.second);
                  if(sigma!=0) {
                      addToLitsPerSupport(sigma, lit.first, lit.second);
                  }
                  else {
                      typedef pair<SysInt,DomainInt> temptype;
                      MAKE_STACK_BOX(newsupportbox, temptype, vars.size()); 
                      bool foundsupport=findNewSupportList(newsupportbox, lit.first, lit.second);
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
    
    #define ADDTOASSIGNMENTFL(var, val) assignment.push_back(make_pair(var,val));
  
      bool findNewSupportList(box<pair<int, DomainInt> >& assignment, int var, DomainInt val) {
        D_ASSERT(vars[var].inDomain(val));
        
        vector<vector<DomainInt>* > & tups=tuple_lists[var][checked_cast<SysInt>(val-vars[var].getInitialMin())];
        
        int cur=tuple_list_pos[var][checked_cast<SysInt>(val-vars[var].getInitialMin())];
        int numtups=tups.size();
        int numvars=vars.size();
        for( ; cur<numtups; cur++) {
            vector<DomainInt>& tup=*(tups[cur]);
            bool valid=true;
            for(int i=0; i<numvars; i++) {
                if(!vars[i].inDomain(tup[i])) {
                    valid=false;
                    break;
                }
            }
            if(valid) {
                // Copy into the box
                for(int i=0; i<numvars; i++) {
                    assignment.push_back(make_pair(i,tup[i]));
                }
                tuple_list_pos[var][checked_cast<SysInt>(val-vars[var].getInitialMin())]=cur;
                return true;
            }
        }
        return false;
    }
    

    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
       for(SysInt i = 0; i < checked_cast<SysInt>(data->size()); ++i)
        {
            if(std::equal(v, v + array_size, data->get_tupleptr(i)))
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
            vector<pair<SysInt,DomainInt> > temp;
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
        if(data->size() == 0)
        {
            getState(stateObj).setFailed(true);
            return;
        }

        // For each literal, find a support for it or delete it. 
        for(SysInt var=0; var<vars.size(); var++) {
            for(DomainInt val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
                if(vars[var].inDomain(val)) {
                    
                    // From here is cut-and-paste from propagate.
                    Support* sigma=seekInferableSupport(var, val);
                      if(sigma!=0) {
                          addToLitsPerSupport(sigma, var, val);
                      }
                      else {
                          typedef pair<SysInt,DomainInt> temptype;
                          MAKE_STACK_BOX(newsupportbox, temptype, vars.size());
                          bool foundsupport=findNewSupportList(newsupportbox, var, val);
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
                    D_ASSERT(supportListPerLit[var][checked_cast<SysInt>(val-dom_min)].next[var]!=0);
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
    
    
    Support* seekInferableSupport(SysInt var, DomainInt val) {
        const SysInt validx=checked_cast<SysInt>(val-dom_min);
        while(supportListPerLit[var][validx].next[var]!=0) {
            Support* temp=supportListPerLit[var][validx].next[var];
            vector<pair<SysInt, DomainInt> >& literals=temp->literals;
            bool lost=false;
            for(SysInt i=0; i<vars.size(); i++) {
                pair<SysInt, DomainInt> lit=literals[i];
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


