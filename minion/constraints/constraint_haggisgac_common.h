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