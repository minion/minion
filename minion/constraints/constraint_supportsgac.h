// LIST BASED CODE WONT BE WORKING

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

// Switches on the zeroLits array. 
// This flag is a small slowdown on qg-supportsgac-7-9 -findallsols
// 
#define SupportsGACUseZeroVals true

template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint, Backtrackable
{
    struct Support ; 

    struct SupportCell { 
	    int literal ; 
	    Support* sup ; 
	    SupportCell* next ; 
	    SupportCell* prev ; 
    };

    struct Literal { 
	int var ; 
	int val ;
	SupportCell* supportCellList; 
//	Literal() { supportCellList = 0 ;} 
    };

    struct Support {
        vector<SupportCell> supportCells ;   // Size can't be more than r, but can be less.
        
	int arity; 		// could use vector.size() but don't want to destruct SupportCells when arity decreases
				// or reconstruct existing ones when it increases.
        
	Support* nextFree ; // for when Support is in Free List.
        
        
        Support()
        {
            supportCells.resize(0);
	    arity=0;
	    nextFree=0;
        }
    };
    
    virtual string constraint_name()
    {
        return "ShortSupportsGAC";
    }
    
    VarArray vars;
    
    vector<pair<int,int> > literalsScratch;   // used instead of per-Support list, as scratch space
    
    int numvals;
    int numlits;
    
    // Counters
    int supports;   // 0 to rd.  
    vector<int> supportsPerVar;
    vector<int> supportsPerLit;
    
    vector<Literal>  literalList;
    vector<int> firstLiteralPerVar;
    
    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    #if SupportsGACUseZeroVals
    vector<vector<int> > zeroLits;
    vector<char> inZeroLits;  // is a literal in zeroVals
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
	int numvars = vars.size(); 
	
	// literalsScratch.reserve(numvars);

	literalsScratch.resize(0);

        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(numvars, 0);
        
	firstLiteralPerVar.resize(numvars); 

	int litCounter = 0 ; 
	numvals = 0 ; 		// only used now by tuple list stuff

        for(int i=0; i<numvars; i++) {

	    firstLiteralPerVar[i] = litCounter; 
	    int thisvalmin = vars[i].getInitialMin();
	    int numvals_i = vars[i].getInitialMax()-thisvalmin+1;
	    if(numvals_i > numvals) numvals = numvals_i;
	    litCounter += numvals_i; 
        }

	literalList.resize(litCounter); 
	supportsPerLit.resize(litCounter,0); 

	litCounter = 0 ; 
        for(int i=0; i<numvars; i++) {
	    int thisvalmin = vars[i].getInitialMin();
	    int numvals_i = vars[i].getInitialMax()-thisvalmin+1;
            for(int j=0; j<numvals_i; j++) {
		    literalList[litCounter].var = i; 
		    literalList[litCounter].val = j+thisvalmin; 
		    literalList[litCounter].supportCellList = 0;
		    literalList[litCounter].nextPrimeLit = -1;
		    litCounter++;
	    }
        }

	numlits = litCounter;
        
        
        #if SupportsGACUseZeroVals
        zeroLits.resize(numvars);
        for(int i=0 ; i < numvars ; i++) {
	    int numvals_i = vars[i].getInitialMax()- vars[i].getInitialMin()+1; 
            zeroLits[i].reserve(numvals_i);  // reserve the maximum length.
            zeroLits[i].resize(0); 
	    int thisvarstart = firstLiteralPerVar[i];
            for(int j=0 ; j < numvals_i; j++) zeroLits[i].push_back(j+thisvarstart);
        }
        inZeroLits.resize(numlits,true); 
        #endif
        
        // Partition
        varsPerSupport.resize(vars.size());
        varsPerSupInv.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            varsPerSupport[i]=i;
            varsPerSupInv[i]=i;
        }
        
        // Start with 1 cell in partition, for 0 supports. 
        supportNumPtrs.resize(numlits+1);
        supportNumPtrs[0]=0;
        for(int i=1; i<= numlits; i++) supportNumPtrs[i]=vars.size();
        
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
                    

	// Want to find all active support objects so we can delete them 
        for(int lit=0; lit<numlits; lit++) {
               SupportCell* supCell = literalList[lit].supportCellList; 

	      // cout << "     destructor 2: sup*= " << sup << endl ; 
                while(supCell!=0) {
                    myset.insert(supCell->sup);	// may get inserted multiple times but it's a set.
		    supCell = supCell->next;
            }
        }
        
        // Go through supportFreeList
        
        while(supportFreeList!=0) {
            Support* sup=supportFreeList;
            supportFreeList=sup->nextFree;
            myset.insert(sup);
        }
        
	// Anything remaining on bracktrack stack
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
            // o<< rec.sup->literals;
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
                addSupportInternal(temp.sup);
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
    
    // don't need argument?   Just use litlist member?  
    //
    //Support* addSupport(box<pair<int, DomainInt> >* litlist)
    void addSupport()
    {
       Support* newsup = getFreeSupport(); 
       vector<SupportCell>& supCells=newsup->supportCells;
       int oldsize = supCells.size() ;
       int newsize = literalsScratch.size() ;

       newsup->arity = newsize;

       if(newsize > oldsize) { 
	       supCells.resize(newsize) ; 
	       // make sure pointers to support cell are correct
	       // need only be done once as will always point to
	       // its own support
	       for(int i=oldsize; i < newsize ; i++) { 
		       supCells[i].sup = newsup; 
	       }
       }

       for(int i=0; i<newsize ; i++) {
            int var=literalsScratch[i].first;
	    int valoriginal=literalsScratch[i].second;
            int lit=firstLiteralPerVar[var]+valoriginal-vars[var].getInitialMin();
	    supCells[i].literal = lit;
       }
	// now have enough supCells, and sup and literal of each is correct

        addSupportInternal(newsup);
        struct BTRecord temp;
        temp.is_removal=false;
        temp.sup=newsup;
        backtrack_stack.push_back(temp);
        // return newsup;
    }
    
    // these guys can be void 
    //
    //
    
    // Takes a support which has: 
    //  	arity correct
    //  	supCells containing at least arity elements
    //  	each supCells[i[ in range has 
    //  	      literal correct
    //  	      sup correct

    void addSupportInternal(Support* sup_internal)
    {
        // add a new support given literals but not pointers in place
        
        
        //cout << "Adding support (internal) :" << litlist_internal << endl;
        //D_ASSERT(litlist_internal.size()>0);  
	//// It should be possible to deal with empty supports, but currently they wil
        // cause a memory leak. 
        
        vector<SupportCell>& supCells=sup_internal->supportCells;

	int litsize = sup_internal->arity;

        for(int i=0; i<litsize; i++) {
            
	    int lit=supCells[i].literal;
	    int var=literalList[lit].var;
            
            //update counters
            supportsPerVar[var]++;
            supportsPerLit[lit]++;  
            // Stitch it into the start of literalList.supportCellList
            
            supCells[i].prev = 0;
            supCells[i].next = literalList[lit].supportCellList;  
            if(literalList[lit].supportCellList!=0) {
                literalList[lit].supportCellList->prev = &(supCells[i]);
            }
	    else { 
            // Attach trigger if this is the first support containing var,val.
                attach_trigger(var, literalList[lit].val, lit);
            }
	    literalList[lit].supportCellList = &(supCells[i]);
            
            // Update partition
            // swap var to the end of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]]-1]);
            // Move the boundary so var is now in the higher cell.
            supportNumPtrs[supportsPerVar[var]]--;
        }
        supports++;
        
        //printStructures();
        
        // return sup_internal;
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
        
        vector<SupportCell>& supCells=sup->supportCells;
	int supArity = sup->arity; 
        //cout << "Removing support (internal) :" << litlist << endl;
        
	
        for(int i=0; i<supArity; i++) {

	    SupportCell& supCell = supCells[i];
	    int lit=supCell.literal;
            int var=literalList[lit].var ;

	    // unstitch cell from list 
	    if(supCell.prev != 0){ 
		    supCell.prev->next = supCell.next;
	    }
	    if(supCell.next!=0){
		    supCell.next->prev = supCell.prev;
	    }
            
            // decrement counters
            supportsPerVar[var]--;
            supportsPerLit[lit]--;
            D_ASSERT(supportsPerLit[lit] >= 0);
            
            #if SupportsGACUseZeroVals
            if(supportsPerLit[lit]==0) {
                if(!inZeroLits[lit]) {
                    inZeroLits[lit]=true;
                    zeroLits[var].push_back(lit);  
                }
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
            // Stick it on the free list 
            sup->nextFree=supportFreeList;
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
        #if SupportsGACUseZeroLits
        cout << "zeroLits:" << zeroLits << endl;
        cout << "inZeroLits:" << inZeroLits << endl;
        #endif
	/*
        
        cout << "Supports for each literal:"<<endl;
        for(int var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                cout << "Value: "<<val<<endl;
                Support* sup=supportListPerLit[var][val-vars[var].getInitialMin()].next[var];
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
	*/
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
				int lit=firstLiteralPerVar[var]+val-vars[var].getInitialMin();
		    #else
		    for(int j=0; j<zeroLits[var].size(); j++) {
			int lit=zeroLits[var][j];
			if(supportsPerLit[lit] > 0){
		    // No longer a zero val. remove from vector.
			    zeroLits[var][j]=zeroLits[var][zeroLits[var].size()-1];
			    zeroLits[var].pop_back();
			    inZeroLits[lit]=false;
			    j--;
			    continue;
			}
			int val=literalList[lit].val;
		    #endif
			
			if(vars[var].inDomain(val) && supportsPerLit[lit]==0) {
			    // val has no support. Find a new one. 
			    bool foundsupport=findNewSupport(var, val);
			    
			    if(!foundsupport) {
				vars[var].removeFromDomain(val);
			    }
			    else {
				addSupport();
				
				#if SupportsGACUseZeroVals
				if(supportsPerLit[lit]>0) {
				    // No longer a zero lit. remove from vector.
				    zeroLits[var][j]=zeroVals[var][zeroVals[var].size()-1];
				    zeroLits[var].pop_back();
				    inZeroLits[lit]=false;
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
    }
    
    inline void updateCounters(int lit) {

        SupportCell* supCellList = literalList[lit].supportCellList ;


        while(supCellList != 0) {
            SupportCell* next=supCellList->next;
            deleteSupport(supCellList->sup);
            supCellList=next;
        }
    }
    
    
    #if SupportsGACUseDT
        int dynamic_trigger_count() { 
            return literalList.size();
        }
    #endif
    
  inline void attach_trigger(int var, int val, int lit)
  {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+lit;
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  inline void detach_trigger(int lit)
  {
      //P("Detach Triggers");
      
      // D_ASSERT(supportListPerLit[var][val-vars[var].getInitialMin()].next[var] == 0);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      dt=dt+lit;
      releaseTrigger(stateObj, dt );   // BT_CALL_BACKTRACK
  }
    
  virtual void propagate(int prop_var, DomainDelta)
  {
  /* 
   Probably won't work
   */
    cout << "Have given up trying to make this work without dynamic triggers" << endl ;
    /*
     *
    D_ASSERT(prop_var>=0 && prop_var<vars.size());
    // Really needs triggers on each value, or on the supports. 
    
    //printStructures();
    D_ASSERT(!SupportsGACUseDT);  // Should not be here if using dynamic triggers.
    
    for(int val=vars[prop_var].getInitialMin(); val<=vars[prop_var].getInitialMax(); val++) {
        if(!vars[prop_var].inDomain(val) && supportListPerLit[prop_var][val-vars[prop_var].getInitialMin()].next[prop_var]!=0) {
            updateCounters(prop_var, val);
        }
    }
    
    findSupports();
  }
  
    virtual void propagate(DynamicTrigger* dt)
  {
      int lit=dt-dynamic_trigger_start();
      
    //  cout << "Propagate called: var= " << var << "val = " << val << endl;
      //printStructures();
      
      updateCounters(lit);
      
      findSupports();
  }

    
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
    
    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
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

    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
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
                            literalsScratch.push_back(make_pair(i, j));
                            literalsScratch.push_back(make_pair(vars.size()-2, i));
                            literalsScratch.push_back(make_pair(vars.size()-1, j));
                            for(int k=0; k<vars.size()-2; k++) {
                                if(k!=i) {
                                    if(k==var)
                                        literalsScratch.push_back(make_pair(k, val));
                                    else
                                        literalsScratch.push_back(make_pair(k, vars[k].getMin()));
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
                    literalsScratch.push_back(make_pair(vars.size()-2, val));
                    literalsScratch.push_back(make_pair(vars.size()-1, i));
                    literalsScratch.push_back(make_pair(val, i));
                    for(int k=0; k<vars.size()-2; k++) {
                        if(k!=val) literalsScratch.push_back(make_pair(k, vars[k].getMin()));
                    }
                    return true;
                }
            }
            
        }
        else if(var==vars.size()-1) {
            // The result variable.
            for(int i=0; i<vars.size()-2; i++) {
                if(vars[i].inDomain(val) && idxvar.inDomain(i)) {
                    literalsScratch.push_back(make_pair(vars.size()-2, i));
                    literalsScratch.push_back(make_pair(vars.size()-1, val));
                    literalsScratch.push_back(make_pair(i, val));
                    for(int k=0; k<vars.size()-2; k++) {
                        if(k!=i) literalsScratch.push_back(make_pair(k, vars[k].getMin()));
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
    
    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
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
    
    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
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
    
    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
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

    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
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

    // bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
    bool findNewSupport(int var, int val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        int i=vars[4].getAssignedValue();
        int j=vars[5].getAssignedValue();
        
        // object i below object j.
        if(vars[1].getMin()+i <=vars[3].getMax()) {
            if(var==1) {
                if(val+i<=vars[3].getMax()) { 
                    literalsScratch.push_back(make_pair(1, val));
                    literalsScratch.push_back(make_pair(3, vars[3].getMax()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(vars[1].getMin()+i<=val) {
                    literalsScratch.push_back(make_pair(1, vars[1].getMin()));
                    literalsScratch.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(1, vars[1].getMin()));
                literalsScratch.push_back(make_pair(3, vars[3].getMax()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i above object j
        if(vars[3].getMin()+j <= vars[1].getMax()) {
            if(var==1) {
                if(vars[3].getMin()+j <= val) {
                    literalsScratch.push_back(make_pair(1, val));
                    literalsScratch.push_back(make_pair(3, vars[3].getMin()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(val+j <= vars[1].getMax())
                {
                    literalsScratch.push_back(make_pair(1, vars[1].getMax()));
                    literalsScratch.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(1, vars[1].getMax()));
                literalsScratch.push_back(make_pair(3, vars[3].getMin()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i left of object j.
        if(vars[0].getMin()+i <=vars[2].getMax()) {
            if(var==0) {
                if(val+i <=vars[2].getMax()) {
                    literalsScratch.push_back(make_pair(0, val));
                    literalsScratch.push_back(make_pair(2, vars[2].getMax()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(vars[0].getMin()+i <=val) {
                    literalsScratch.push_back(make_pair(0, vars[0].getMin()));
                    literalsScratch.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(0, vars[0].getMin()));
                literalsScratch.push_back(make_pair(2, vars[2].getMax()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        // object i right of object j
        if(vars[2].getMin()+j <= vars[0].getMax()) {
            if(var==0) {
                if(vars[2].getMin()+j <= val) {
                    literalsScratch.push_back(make_pair(0, val));
                    literalsScratch.push_back(make_pair(2, vars[2].getMin()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(val+j <= vars[0].getMax()) {
                    literalsScratch.push_back(make_pair(0, vars[0].getMax()));
                    literalsScratch.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(0, vars[0].getMax()));
                literalsScratch.push_back(make_pair(2, vars[2].getMin()));
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
            return new Support();
        }
        else {
            Support* temp=supportFreeList;
            supportFreeList=supportFreeList->nextFree;
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


