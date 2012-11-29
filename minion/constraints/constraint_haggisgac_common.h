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