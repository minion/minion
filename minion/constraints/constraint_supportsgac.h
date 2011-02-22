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

// Default will be List.   
// If any special case is defined list will be switched off
// If two options given compile errors are expected to result.

#define UseElementShort false
#define UseElementLong false
#define UseLexLeqShort false
#define UseLexLeqLong false
#define UseSquarePackingShort false
#define UseSquarePackingLong false
#define UseList true

#ifdef SUPPORTSGACELEMENT
#undef UseElementShort
#undef UseList
#define UseElementShort true
#define UseList false
#endif

#ifdef SUPPORTSGACELEMENTLONG
#undef UseElementLong
#undef UseList
#define UseElementLong true
#define UseList false
#endif

#ifdef SUPPORTSGACLEX
#undef UseLexLeqShort
#undef UseList
#define UseLexLeqShort true
#define UseList false
#endif

#ifdef SUPPORTSGACLEXLONG
#undef UseLexLeqLong
#undef UseList
#define UseLexLeqLong true
#define UseList false
#endif

#ifdef SUPPORTSGACSQUAREPACK
#undef UseSquarePackingShort
#undef UseList
#define UseSquarePackingShort true
#define UseList false
#endif

#ifdef SUPPORTSGACSQUAREPACKLONG
#undef UseSquarePackingLong
#undef UseList
#define UseSquarePackingLong true
#define UseList false
#endif

#ifdef SUPPORTSGACLIST
#undef UseList
#define UseList true
#endif




// The algorithm iGAC or short-supports-gac

// Does it place dynamic triggers for the supports.
#define SupportsGACUseDT true

// Switches on the zeroVals array. 
// This flag is a small slowdown on qg-supportsgac-7-9 -findallsols
// 
#define SupportsGACUseZeroVals true

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
    int numlits;
    
    // Counters
    int supports;   // 0 to rd.  
    vector<int> supportsPerVar;

    vector<pair<int,int> > litsWithLostExplicitSupport;
    vector<int> varsWithLostImplicitSupport;
    
    // 2d array (indexed by var then val) of sentinels,
    // at the head of list of supports. 
    // Needs a sentinel at the start so that dlx-style removals work correctly.
    vector<vector<Support> >  supportListPerLit;
    
    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    #if SupportsGACUseZeroVals
    vector<vector<int> > zeroVals;
    vector<vector<char> > inZeroVals;  // is a var/val in zeroVals
    #endif
    
    // Partition of variables by number of supports.
    vector<int> varsPerSupport;    // Permutation of the variables
    vector<int> varsPerSupInv;   // Inverse mapping of the above.
    
    vector<int> supportNumPtrs;   // rd+1 indices into varsPerSupport representing the partition
    
    Support* supportFreeList;       // singly-linked list of spare Support objects.
    
    vector<vector<vector<vector<pair<int,int> > > > > tuple_lists;  // tuple_lists[var][val] is a vector 
    // of short supports for that var, val. Includes any supports that do not contain var at all.
    
    vector<vector<int> > tuple_list_pos;    // current position in tuple_lists (for each var and val). Wraps around.
    //
    ////////////////////////////////////////////////////////////////////////////
    // Ctor
    
    ShortSupportsGAC(StateObj* _stateObj, const VarArray& _var_array, TupleList* tuples) : AbstractConstraint(_stateObj), 
    vars(_var_array), supportFreeList(0)
    {
        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        numlits = vars[0].getInitialMax() - vars[0].getInitialMin() + 1;

	numvals==numlits;

        for(int i=vars.size()-1; i > 0 ; i--) {
	    int numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
	    numlits += numvals_i; 
	    if(numvals_i > numvals) numvals = numvals_i;
        }

        // Initialise counters
        supports=0;
        supportsPerVar.resize(vars.size(), 0);
        
        supportListPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
	    // int numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
	     // cout << "     i " << i << " Initial Max " << vars[i].getInitialMax() << endl ; 
	    int numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
            supportListPerLit[i].resize(numvals_i);  // blank Support objects.
            for(int j=0; j<numvals_i; j++) {
		    supportListPerLit[i][j].next.resize(vars.size());
	      // cout << "     i j SupportListPerLit[var][val].next = " << i << " " << j << " " << supportListPerLit[i][j].next << endl ; 
	    }
        }
        
        #if SupportsGACUseZeroVals
        zeroVals.resize(vars.size());
        inZeroVals.resize(vars.size());
        for(int i=vars.size()-1 ; i >= 0; i--) {
	    int numvals_i = vars[i].getInitialMax()- vars[i].getInitialMin()+1; // vars[i].getInitialMin()+1;
            zeroVals[i].reserve(numvals_i);  // reserve the maximum length.
            for(int j=vars[i].getInitialMin(); j<=vars[i].getInitialMax(); j++) zeroVals[i].push_back(j);
            inZeroVals[i].resize(numvals_i, true);
        }
        #endif
	
	// Lists (vectors) of literals/vars that have lost support.
	// Set this up to insist that everything needs to have support found for it on full propagate.
	
        litsWithLostExplicitSupport.reserve(vars.size()*numvals); // max poss size, not necessarily optimal choice here
        varsWithLostImplicitSupport.reserve(vars.size());
       
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
        
        // Extract short supports from tuples if necessary.
        if(tuples->size()>1) {
            cout << "Tuple list passed to supportgac constraint should only contain one tuple, encoding a list of short supports." << endl; 
            abort();
        }
        if(tuples->size()==1) {
            vector<DomainInt> encoded = tuples->get_vector(0);
            vector<vector<pair<int, int> > > shortsupports;
            vector<pair<int, int> > temp;
            for(int i=0; i<encoded.size(); i=i+2) {
                if(encoded[i]==-1) {
                    // end of a short support.
                    if(encoded[i+1]!=-1) {
                        cout << "Split marker is -1,-1 in tuple for supportsgac." << endl;
                        abort();
                    }
                    shortsupports.push_back(temp);
                    temp.clear();
                }
                else
                {
                    if(encoded[i]<0 || encoded[i]>=vars.size()) {
                        cout << "Tuple passed into supportsgac does not correctly encode a set of short supports." << endl;
                        abort();
                    }
                    temp.push_back(make_pair(encoded[i], encoded[i+1])); 
                }
            }
            if(encoded[encoded.size()-2]!=-1 || encoded[encoded.size()-1]!=-1) {
                cout << "Last -1,-1 marker missing from tuple in supportsgac."<< endl;
                abort();
            }
            
            tuple_lists.resize(vars.size());
            tuple_list_pos.resize(vars.size());

            for(int var=0; var<vars.size(); var++) {
                tuple_lists[var].resize(numvals);
                tuple_list_pos[var].resize(numvals, 0);
                
                for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                    // get short supports relevant to var,val.
                    for(int i=0; i<shortsupports.size(); i++) {
                        bool varin=false;
                        bool valmatches=true;
                        for(int j=0; j<shortsupports[i].size(); j++) {
                            if(shortsupports[i][j].first==var) {
                                varin=true;
                                if(shortsupports[i][j].second!=val) {
                                    valmatches=false;
                                }
                            }
                        }
                        
                        if(!varin || valmatches) {
                            // If the support doesn't include the var, or it 
                            // does include var,val then add it to the list.
                            tuple_lists[var][val-vars[var].getInitialMin()].push_back(shortsupports[i]);
                        }
                    }
                }
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Dtor
    
    virtual ~ShortSupportsGAC() {
        //printStructures();
        set<Support*> myset;

	/* 
        for(int i=0; i<vars.size(); i++) {
	    cout << "     i " << i << " Initial Max " << vars[i].getInitialMax() << endl ; 
	    int numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
            for(int j=0; j<numvals_i; j++) {
	      cout << "     i j SupportListPerLit[var][val].next = " << i << " " << j << " " << supportListPerLit[i][j].next << endl ; 
	    }
        }
	*/
        
        // Go through supportFreeList
        for(int var=0; var<vars.size(); var++) {
	  // cout << "  destructor 2: var= " << var << " original var " << vars[var] << endl ; 
            for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
	      // cout << "     destructor 2: val= " << val << "val - vars[var].getInitialMin() = " << val - vars[var].getInitialMin() << endl ; 
	      // cout << "     SupportListPerLit[var][val].next = " << supportListPerLit[var][val-vars[var].getInitialMin()].next << endl ; 
                Support* sup = supportListPerLit[var][val-vars[var].getInitialMin()].next[var];
	      // cout << "     destructor 2: sup*= " << sup << endl ; 
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
                    sup=supportListPerLit[var][val-vars[var].getInitialMin()].next[var];
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
	    int valoriginal=temp.second;
            int val=valoriginal-vars[var].getInitialMin();
	    
            // Attach trigger if this is the first support containing var,val.
            if(SupportsGACUseDT && supportListPerLit[var][val].next[var]==0) {
                attach_trigger(var, valoriginal);
            }
            
            // Stitch it into supportListPerLit
            sup_internal->prev[var]= &(supportListPerLit[var][val]);
            sup_internal->next[var]= supportListPerLit[var][val].next[var];
            supportListPerLit[var][val].next[var]=sup_internal;
            if(sup_internal->next[var] != 0)
                sup_internal->next[var]->prev[var]=sup_internal;
            
            //update counters
            supportsPerVar[var]++;
            
            
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
        
	// oldIndex is where supportsPerVar = numsupports used to be 
	// Off by 1 error?

	int oldIndex  = supportNumPtrs[supports];
	
        for(int i=0; i<litlist.size(); i++) {
            int var=litlist[i].first;
            int valoffset=litlist[i].second-vars[var].getInitialMin();

	    D_ASSERT(prev[var]!=0);
            prev[var]->next[var]=next[var];
            if(next[var]!=0) {
                next[var]->prev[var]=prev[var];
            }
            
            // decrement counters
            supportsPerVar[var]--;
            
	    // I believe that each literal can only be marked once here in a call to update_counters.
	    // so we should be able to push it onto a list
	    //
	    // As long as we do not actually call find_new_support.
	    // So probably should shove things onto a list and then call find supports later

            if(supportListPerLit[var][valoffset].next[var] == 0){
		    // Surely don't need to update lost supports on backtracking in non-backtrack-stable code?
		if (!Backtracking && supportsPerVar[var] == (supports - 1)) {	// since supports not decremented yet
			litsWithLostExplicitSupport.push_back(make_pair(var,litlist[i].second));
		}
                #if SupportsGACUseZeroVals
			// Still need to add to zerovals even if above test is true
			// because we might find a new implicit support, later lose it, and then it will 
			// be essential that it is in zerovals.  Obviously if an explicit support is 
			// found then it will later get deleted from zerovals.
		if(!inZeroVals[var][valoffset]) {
                    inZeroVals[var][valoffset]=true;
                    zeroVals[var].push_back(litlist[i].second);
                }
                #endif
            // Remove trigger if this is the last support containing var,val.
               if(SupportsGACUseDT) { detach_trigger(var, litlist[i].second); }
            }
            
            
            // Update partition
            // swap var to the start of its cell.  
	    // This plays a crucial role in moving together the vars which previously
	    // had 1 less than numsupports and soon will have numsupports.

            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]+1]]);
            // Move the boundary so var is now in the lower cell.
            supportNumPtrs[supportsPerVar[var]+1]++;

	    
        }
        supports--;
	
	// For following code it is essential that partition swaps compress 
	// vars together which used to have SupportsPerVar[i] = supports-1 and 
	// now have supportsPerVar[i] = supports (because supports has been decremented
	// 
	//
	    //
	    // Similarly to the above, each var can only be added to this list once per call to update_counters
	    // Because it can only lose its last implicit support once since we are only deleting supports.
	    //
	
	// I hope we only need to do this when NOT backtracking, at least for non backtrack-stable version
	// When we backtrack we will add supports which did support it so there is no need to find new supports

// 	cout << supportNumPtrs[supports] << " " << oldIndex << endl;
	
	if (!Backtracking) {
		for(int i=supportNumPtrs[supports]; i < oldIndex; i++) { 
			varsWithLostImplicitSupport.push_back(varsPerSupport[i]);
		}
	} 
	else { 
            // We are Backtracking 
	    // Can re-use the support when it is removed by BT. 
            // Stick it on the free list using next[0] as the next ptr.
            sup->next[0]=supportFreeList;
            supportFreeList=sup;
        }
        // else can't re-use it because a ptr to it is on the BT stack. 
    }

    BOOL hasNoKnownSupport(int var,int val) {
	    //
	    // Either implicitly supported or counter is non zero
	    // Note that even if we have an explicit support which may be invalid, we can return true
	    // i.e. code does not guarantee that it has a valid support, only that it has a support.
	    // If we have no valid supports then (if algorithms are right) we will eventually delete
	    // the last known valid support and at that time start looking for a new one.

	    return supportsPerVar[var] == supports && (supportListPerLit[var][val-vars[var].getInitialMin()].next[var] == 0);
    }
    //
    ////////////////////////////////////////////////////////////////////////////
    // 
    void printStructures()
    {
        cout << "PRINTING ALL DATA STRUCTURES" <<endl;
        cout << "supports:" << supports <<endl;
        cout << "supportsPerVar:" << supportsPerVar << endl;
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
        cout << "inZeroVals:" << inZeroVals << endl;
        #endif
        
        cout << "Supports for each literal:"<<endl;
        for(int var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
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

    void findSupportsIncrementalHelper(int var, int val) { 

	    typedef pair<int,DomainInt> temptype;
	    MAKE_STACK_BOX(newsupportbox, temptype, vars.size()); 
	    bool foundsupport=findNewSupport(newsupportbox, var, val);
	    
	    if(!foundsupport) {
		vars[var].removeFromDomain(val);	
			// note we are not doing this internally, 
			// i.e. trying to update counters etc. 
			// So counters won't have changed until we are retriggered on the removal
	    }
	    else {
		addSupport(&newsupportbox);
	    }
    }
    
    void findSupportsIncremental()
    {
	// For list of vars which have lost their last implicit support
	// do this ... 
	// but don't need to redo if we have stored that list ahead of time 
	//  ... and we can check if it still has support 
	//
        // For each variable where the number of supports is equal to the total...

	for(int i=litsWithLostExplicitSupport.size()-1; i >= 0; i--) { 
	    int var=litsWithLostExplicitSupport[i].first;
	    int val=litsWithLostExplicitSupport[i].second;
	    
	    litsWithLostExplicitSupport.pop_back(); // actually probably unnecessary - will get resized to 0 later
	    
	    if(vars[var].inDomain(val) && hasNoKnownSupport(var,val)) {
		    findSupportsIncrementalHelper(var,val);
	    }
	}

        for(int i = varsWithLostImplicitSupport.size()-1; i >= 0; i--) { 

            int var= varsWithLostImplicitSupport[i];
	    varsWithLostImplicitSupport.pop_back(); // actually probably unnecessary - will get resized to 0 later

	    if (supportsPerVar[var] == supports) { 	// otherwise has found implicit support in the meantime
		    #if !SupportsGACUseZeroVals
		    for(int val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
		    #else
		    for(int j=0; j<zeroVals[var].size(); j++) {
			int val=zeroVals[var][j];
                        if(supportListPerLit[var][val-vars[var].getInitialMin()].next[var] != 0){
			    // No longer a zero val. remove from vector.
			    zeroVals[var][j]=zeroVals[var][zeroVals[var].size()-1];
			    zeroVals[var].pop_back();
			    inZeroVals[var][val-vars[var].getInitialMin()]=false;
			    j--;
			    continue;
			}
		    #endif

		    #if !SupportsGACUseZeroVals
			if(vars[var].inDomain(val) && (supportListPerLit[var][val-vars[var].getInitialMin()].next[var] == 0)){
		    #else
			if(vars[var].inDomain(val)) {	// tested supportListPerLit  above
		    #endif
		            findSupportsIncrementalHelper(var,val);
			    // No longer do we remove j from zerovals in this case if support is found.
			    // However this is correct as it can be removed lazily next time the list is traversed
			    // And we might even get lucky and save this small amount of work.
			} // } to trick vim bracket matching
		    }    // } to trick vim bracket matching
		}
	}
    }

    inline void updateCounters(int var, int val) {

        Support* supList = supportListPerLit[var][val-vars[var].getInitialMin()].next[var];

        litsWithLostExplicitSupport.resize(0);   
        varsWithLostImplicitSupport.resize(0);

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
      dt=dt+(var*numvals)+(val-vars[var].getInitialMin());
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  inline void detach_trigger(int var, int val)
  {
      //P("Detach Triggers");
      
      D_ASSERT(supportListPerLit[var][val-vars[var].getInitialMin()].next[var] == 0);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      dt=dt+(var*numvals)+(val-vars[var].getInitialMin());
      releaseTrigger(stateObj, dt );   // BT_CALL_BACKTRACK
  }
    
  virtual void propagate(int prop_var, DomainDelta)
  {
  /* 
   Probably won't work
   */

    D_ASSERT(prop_var>=0 && prop_var<vars.size());
    // Really needs triggers on each value, or on the supports. 
    
    //printStructures();
    D_ASSERT(!SupportsGACUseDT);  // Should not be here if using dynamic triggers.
    
    for(int val=vars[prop_var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
        if(!vars[prop_var].inDomain(val) && supportListPerLit[prop_var][val-vars[prop_var].getInitialMin()].next[prop_var]!=0) {
            updateCounters(prop_var, val);
        }
    }
    
    findSupportsIncremental();
  }
  
    virtual void propagate(DynamicTrigger* dt)
  {
      int pos=dt-dynamic_trigger_start();
      int var=pos/numvals;
      int val=pos-(var*numvals)+vars[var].getInitialMin();

    //  cout << "Propagate called: var= " << var << "val = " << val << endl;
      //printStructures();

      deletedSupports.resize(0);
      
      updateCounters(var, val);
      
      findSupportsIncremental();
  }

    // HERE will need to be changed for backtrack stability, i.e. added even if isAssigned. Or use FL
    
    #define ADDTOASSIGNMENT(var, val) if(!vars[var].isAssigned()) assignment.push_back(make_pair(var,val));
    
    // For full-length support variant:
    #define ADDTOASSIGNMENTFL(var, val) assignment.push_back(make_pair(var,val));
    
    
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

#if UseElementShort
    
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
                            ADDTOASSIGNMENT(i,j);
                            ADDTOASSIGNMENT(vars.size()-2, i);
                            ADDTOASSIGNMENT(vars.size()-1, j);
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
                    ADDTOASSIGNMENT(vars.size()-2, val);
                    ADDTOASSIGNMENT(vars.size()-1, i);
                    ADDTOASSIGNMENT(val, i);
                    return true;
                }
            }
            
        }
        else if(var==vars.size()-1) {
            // The result variable.
            for(int i=0; i<vars.size()-2; i++) {
                if(vars[i].inDomain(val) && idxvar.inDomain(i)) {
                    ADDTOASSIGNMENT(vars.size()-2, i);
                    ADDTOASSIGNMENT(vars.size()-1, val);
                    ADDTOASSIGNMENT(i, val);
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

#endif
    //
    ////////////////////////////////////////////////////////////////////////////
    // ELEMENT - FULL LENGTH TUPLES VERSION.

#if UseElementLong

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


#endif 
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for lexleq
    
#if UseLexLeqShort
    
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
                
                ADDTOASSIGNMENT(i, imin);
                ADDTOASSIGNMENT(j, jmax);
                
                // Do not return, continue along the vector.
                continue;
            }
            
            // CASE 3    It is possible make the pair less.
            if(imin<jmax) {
                if(i==var) {
                    if(val==jmax) {
                        ADDTOASSIGNMENT(i, val);
                        ADDTOASSIGNMENT(j, val);
                        continue;
                    }
                    else if(val>jmax) {
                        return false;
                    }
                    else {   //  val<jmax
                        ADDTOASSIGNMENT(var, val);
                        ADDTOASSIGNMENT(j, jmax);
                        return true;
                    }
                }
                
                if(j==var) {
                    if(val==imin) {
                        ADDTOASSIGNMENT(i, val);
                        ADDTOASSIGNMENT(j, val);
                        continue;
                    }
                    else if(val<imin) {
                        return false;
                    }
                    else {   //  val>imin
                        ADDTOASSIGNMENT(var, val);
                        ADDTOASSIGNMENT(i, imin);
                        return true;
                    }
                }
                
                
                // BETTER NOT TO USE min and max here, should watch something in the middle of the domain...
                //int mid=imin + (jmax-imin)/2;
                //if(vars[i].inDomain(mid-1) && vars[j].inDomain(mid)) {
                //    ADDTOASSIGNMENT(i,mid-1);
                //    ADDTOASSIGNMENT(j,mid);
                //}
                //else {
                    ADDTOASSIGNMENT(i,imin);
                    ADDTOASSIGNMENT(j,jmax);
                
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


#endif 
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Lexleq with full-length supports
    //

#if UseLexLeqLong
    
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
                
                ADDTOASSIGNMENTFL(i, imin);
                ADDTOASSIGNMENTFL(j, jmax);
                
                // Do not return, continue along the vector.
                continue;
            }
            
            // CASE 3    It is possible make the pair less.
            if(imin<jmax) {
                if(i==var) {
                    if(val==jmax) {
                        ADDTOASSIGNMENTFL(i, val);
                        ADDTOASSIGNMENTFL(j, val);
                        continue;
                    }
                    else if(val>jmax) {
                        return false;
                    }
                    else {   //  val<jmax
                        ADDTOASSIGNMENTFL(var, val);
                        ADDTOASSIGNMENTFL(j, jmax);
                        for(int k=i+1; k<vecsize; k++) {
                            PADOUT(k);
                            PADOUT(k+vecsize);
                        }
                        
                        return true;
                    }
                }
                
                if(j==var) {
                    if(val==imin) {
                        ADDTOASSIGNMENTFL(i, val);
                        ADDTOASSIGNMENTFL(j, val);
                        continue;
                    }
                    else if(val<imin) {
                        return false;
                    }
                    else {   //  val>imin
                        ADDTOASSIGNMENTFL(var, val);
                        ADDTOASSIGNMENTFL(i, imin);
                        for(int k=i+1; k<vecsize; k++) {
                            PADOUT(k);
                            PADOUT(k+vecsize);
                        }
                        
                        return true;
                    }
                }
                
                ADDTOASSIGNMENTFL(i,imin);
                ADDTOASSIGNMENTFL(j,jmax);
                for(int k=i+1; k<vecsize; k++) {
                    PADOUT(k);
                    PADOUT(k+vecsize);
                }
                
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
    
#endif
    
#if UseList

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Table of short supports passed in.
    
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(tuple_lists.size()==vars.size());
        
        const vector<vector<pair<int, int> > >& tuplist=tuple_lists[var][val-vars[var].getInitialMin()]; 
        
        int listsize=tuplist.size();
        for(int i=tuple_list_pos[var][val-vars[var].getInitialMin()]; i<listsize; i++) {
            
            int supsize=tuplist[i].size();
            bool valid=true;
            
            for(int j=0; j<supsize; j++) {
                if(! vars[tuplist[i][j].first].inDomain(tuplist[i][j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(int j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tuplist[i][j].first, tuplist[i][j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val-vars[var].getInitialMin()]=i;
                return true;
            }
        }
        
        
        for(int i=0; i<tuple_list_pos[var][val-vars[var].getInitialMin()]; i++) {
            
            int supsize=tuplist[i].size();
            bool valid=true;
            
            for(int j=0; j<supsize; j++) {
                if(! vars[tuplist[i][j].first].inDomain(tuplist[i][j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(int j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tuplist[i][j].first, tuplist[i][j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val-vars[var].getInitialMin()]=i;
                return true;
            }
        }
        return false;
    }
    
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        // argh, how to do this.
        // test with element first
        
        int idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }

#endif
   
#if UseSquarePackingShort

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing.
    // Expects x1,y1, x2,y2, boxsize1, boxsize2 (constant)

    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        int i=vars[4].getAssignedValue();
        int j=vars[5].getAssignedValue();
        
        // If objects totally disjoint in either dimension...
        // x
        if( (vars[0].getMax()+i <= vars[2].getMin()) 
            || (vars[2].getMax()+j <= vars[0].getMin())
        // y
            || (vars[1].getMax()+i <= vars[3].getMin()) 
            || (vars[3].getMax()+j <= vars[1].getMin()) )
        {
            return true;
        }
        
        // object i below object j.

        if(vars[1].getMin()+i <=vars[3].getMax()) {
            if(var==1) {
                if(val+i<=vars[3].getMax()) { 
                    ADDTOASSIGNMENT(1, val);
                    ADDTOASSIGNMENT(3, vars[3].getMax());
                    return true;
                }
            }
            else if(var==3) {
                if(vars[1].getMin()+i<=val) {
                    ADDTOASSIGNMENT(1, vars[1].getMin());
                    ADDTOASSIGNMENT(3, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(1, vars[1].getMin());
                ADDTOASSIGNMENT(3, vars[3].getMax());
                return true;
            }
        }
        
        // object i above object j
        if(vars[3].getMin()+j <= vars[1].getMax()) {
            if(var==1) {
                if(vars[3].getMin()+j <= val) {
                    ADDTOASSIGNMENT(1, val);
                    ADDTOASSIGNMENT(3, vars[3].getMin());
                    return true;
                }
            }
            else if(var==3) {
                if(val+j <= vars[1].getMax())
                {
                    ADDTOASSIGNMENT(1, vars[1].getMax());
                    ADDTOASSIGNMENT(3, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(1, vars[1].getMax());
                ADDTOASSIGNMENT(3, vars[3].getMin());
                return true;
            }
        }
        
        // object i left of object j.
        if(vars[0].getMin()+i <=vars[2].getMax()) {
            if(var==0) {
                if(val+i <=vars[2].getMax()) {
                    ADDTOASSIGNMENT(0, val);
                    ADDTOASSIGNMENT(2, vars[2].getMax());
                    return true;
                }
            }
            else if(var==2) {
                if(vars[0].getMin()+i <=val) {
                    ADDTOASSIGNMENT(0, vars[0].getMin());
                    ADDTOASSIGNMENT(2, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(0, vars[0].getMin());
                ADDTOASSIGNMENT(2, vars[2].getMax());
                return true;
            }
        }
        
        // object i right of object j
        if(vars[2].getMin()+j <= vars[0].getMax()) {
            if(var==0) {
                if(vars[2].getMin()+j <= val) {
                    ADDTOASSIGNMENT(0, val);
                    ADDTOASSIGNMENT(2, vars[2].getMin());
                    return true;
                }
            }
            else if(var==2) {
                if(val+j <= vars[0].getMax()) {
                    ADDTOASSIGNMENT(0, vars[0].getMax());
                    ADDTOASSIGNMENT(2, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(0, vars[0].getMax());
                ADDTOASSIGNMENT(2, vars[2].getMin());
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
#endif

#if UseSquarePackingLong
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing with full-length supports
    // Expects x1,y1, x2,y2, boxsize1, boxsize2 (constant).

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
#endif
    
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
       litsWithLostExplicitSupport.resize(0);
       varsWithLostImplicitSupport.resize(0); 

       for(int i=0; i<vars.size(); i++) { 
	       varsWithLostImplicitSupport.push_back(i);
       }

       findSupportsIncremental();
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


