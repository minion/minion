    virtual AbstractConstraint* reverse_constraint()
    { return forward_check_negation(stateObj, this); }

    struct Support ; 

    struct SupportCell { 
            SysInt literal ; 
            Support* sup ; 
            SupportCell* next ; 
            SupportCell* prev ; 
    };

    struct Literal { 
        SysInt var ; 
        DomainInt val ;
        SupportCell* supportCellList; 
//      Literal() { supportCellList = 0 ;} 
    };



// Methods common to both haggisgac_bt and haggisgac_stable. To be directly included in both those files.

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    D_ASSERT(vars.size() > 0);
    for(DomainInt i = vars[0].getMin(); i <= vars[0].getMax(); ++i)
    {
        literalsScratch.clear();
        if(findNewSupport<true>(0, i))
        {
            for(SysInt j = 0; j < literalsScratch.size(); ++j)
                assignment.push_back(literalsScratch[j]);
            return true;
        }
    }
    return false;
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(vars.size() > 0);
    if(v[0] < vars[0].getInitialMin())
        return false;

    if(v[0] > vars[0].getInitialMax())
        return false;

    const SysInt val_offset = checked_cast<SysInt>(v[0]-vars[0].getInitialMin());
    const vector<vector<pair<SysInt, DomainInt> > * >& tuplist=tuple_lists[0][val_offset];

    for(SysInt i=0; i<tuple_lists[0][val_offset].size(); i++) 
    {
        const vector<pair<SysInt,DomainInt> > & tup=*(tuplist[i]);
        
        SysInt supsize=tup.size();
        bool valid=true;
        
        for(SysInt j=0; j<supsize; j++) {
            if(v[tup[j].first] != tup[j].second) {
                valid=false;
                break;
            }
        }
        
        if(valid) 
            return true;
    }

    return false;
  }

      struct SupportDeref
    {
        template<typename T>
        bool operator()(const T& lhs, const T& rhs)
        #if SupportsGacNoCopyList
        { return *lhs < *rhs; }
        #else
        { return lhs < rhs; }
        #endif
    };

       virtual ~CLASSNAME() {
        //printStructures();
        set<Support*> myset;

        /* 
        for(SysInt i=0; i<vars.size(); i++) {
            cout << "     i " << i << " Initial Max " << vars[i].getInitialMax() << endl ; 
            SysInt numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
            for(SysInt j=0; j<numvals_i; j++) {
              cout << "     i j SupportListPerLit[var][val].next = " << i << " " << j << " " << supportListPerLit[i][j].next << endl ; 
            }
        }
        */
        

        // Want to find all active support objects so we can delete them 
        for(SysInt lit=0; lit<numlits; lit++) {
               SupportCell* supCell = literalList[lit].supportCellList; 

              // cout << "     destructor 2: sup*= " << sup << endl ; 
                while(supCell!=0) {
                    myset.insert(supCell->sup); // may get inserted multiple times but it's a set.
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
        for(SysInt i=0; i<backtrack_stack.size(); i++) {
            if(backtrack_stack[i].sup!=0) {
                myset.insert(backtrack_stack[i].sup);
            }
        }
        
        typename set<Support*>::iterator it;
        for ( it=myset.begin() ; it != myset.end(); it++ ) {
            delete *it;
        }
    }

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

    void init()
    {


        SysInt numvars = vars.size(); 
        
        // literalsScratch.reserve(numvars);

        literalsScratch.resize(0);

        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(numvars, 0);
        
        firstLiteralPerVar.resize(numvars); 

    {
        DomainInt litCounter = 0 ; 
        numvals = 0 ;           // only used now by tuple list stuff

        for(SysInt i=0; i<numvars; i++) {

            firstLiteralPerVar[i] = checked_cast<SysInt>(litCounter); 
            DomainInt numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
            if(numvals_i > numvals) numvals = checked_cast<SysInt>(numvals_i);
            litCounter += numvals_i; 
        }
        literalList.resize(checked_cast<SysInt>(litCounter)); 
    }
    {
        SysInt litCounter = 0 ; 
        for(SysInt i=0; i<numvars; i++) {
            DomainInt thisvalmin = vars[i].getInitialMin();
            DomainInt numvals_i = vars[i].getInitialMax()-thisvalmin+1;
            for(DomainInt j=0; j<numvals_i; j++) {
                    literalList[litCounter].var = i; 
                    literalList[litCounter].val = j+thisvalmin; 
                    literalList[litCounter].supportCellList = 0;
                    litCounter++;
            }
        }

        numlits = litCounter;
    }
        
        zeroLits.resize(numvars);
        for(SysInt i=0 ; i < numvars ; i++) {
            const SysInt numvals_i = checked_cast<SysInt>(vars[i].getInitialMax()- vars[i].getInitialMin()+1); 
            zeroLits[i].reserve(numvals_i);  // reserve the maximum length.
            zeroLits[i].resize(0); 
            SysInt thisvarstart = firstLiteralPerVar[i];
            for(SysInt j=0 ; j < numvals_i; j++) zeroLits[i].push_back(j+thisvarstart);
        }
        inZeroLits.resize(numlits,true); 
        
        // Lists (vectors) of literals/vars that have lost support.
        // Set this up to insist that everything needs to have support found for it on full propagate.
        
        varsWithLostImplicitSupport.reserve(vars.size());
       
        // Partition
        varsPerSupport.resize(vars.size());
        varsPerSupInv.resize(vars.size());
        for(SysInt i=0; i<vars.size(); i++) {
            varsPerSupport[i]=i;
            varsPerSupInv[i]=i;
        }
        
        // Start with 1 cell in partition, for 0 supports. 
        supportNumPtrs.resize(numlits+1);
        supportNumPtrs[0]=0;
        for(SysInt i=1; i<= numlits; i++) supportNumPtrs[i]=vars.size();
        
#if UseList
        // Read in the short supports.
        
        #if UseList && SupportsGacNoCopyList
        vector<vector<pair<SysInt, DomainInt> > * > shortsupports;
        #else
        vector<vector<pair<SysInt, DomainInt> > > shortsupports;
        #endif
        
        const vector<vector<pair<SysInt, DomainInt> > >& tupleRef = (*data->tuplePtr());
        
        for(SysInt i=0; i<tupleRef.size(); i++) {
            
            #if UseList && SupportsGacNoCopyList
            shortsupports.push_back(new vector<pair<SysInt, DomainInt> >(tupleRef[i]));
            #else
            shortsupports.push_back(tupleRef[i]);
            #endif
        }

        
        // Sort it. Might not work when it's pointers.
        for(SysInt i=0; i<shortsupports.size(); i++) {
            // Sort each short support
            #if UseList && SupportsGacNoCopyList
            sort(shortsupports[i]->begin(), shortsupports[i]->end());
            #else
            sort(shortsupports[i].begin(), shortsupports[i].end());
            #endif
        }
        sort(shortsupports.begin(), shortsupports.end(), SupportDeref());
        
        tuple_lists.resize(vars.size());
        tuple_list_pos.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            SysInt domsize = checked_cast<SysInt>(vars[var].getInitialMax()-vars[var].getInitialMin()+1);
            tuple_lists[var].resize(domsize);
            tuple_list_pos[var].resize(domsize, 0);
            
            for(DomainInt val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                // get short supports relevant to var,val.
                for(SysInt i=0; i<shortsupports.size(); i++) {
                    bool varin=false;
                    bool valmatches=true;
                    
                    #if SupportsGacNoCopyList
                    vector<pair<SysInt, DomainInt> > & shortsup=*(shortsupports[i]);
                    #else
                    vector<pair<SysInt, DomainInt> > & shortsup=shortsupports[i];
                    #endif
                    
                    for(SysInt j=0; j<shortsup.size(); j++) {
                        if(shortsup[j].first==var) {
                            varin=true;
                            if(shortsup[j].second!=val) {
                                valmatches=false;
                            }
                        }
                    }
                    
                    if(!varin || valmatches) {
                        // If the support doesn't include the var, or it 
                        // does include var,val then add it to the list.
                        tuple_lists[var][checked_cast<SysInt>(val-vars[var].getInitialMin())].push_back(shortsupports[i]);
                    }
                }
            }
        }
#endif
    }