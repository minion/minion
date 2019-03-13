virtual AbstractConstraint* reverseConstraint() {
  return forwardCheckNegation(this);
}

struct Support;

struct SupportCell {
  SysInt literal;
  Support* sup;
  SupportCell* next;
  SupportCell* prev;
};

struct Literal {
  SysInt var;
  DomainInt val;
  SupportCell* supportCellList;
  //      Literal() { supportCellList = 0 ;}
};

// Methods common to both haggisgac_bt and haggisgac_stable. To be directly
// included in both those files.

virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
  D_ASSERT(vars.size() > 0);
  for(DomainInt i = vars[0].min(); i <= vars[0].max(); ++i) {
    literalsScratch.clear();
    if(findNewSupport<true>(0, i)) {
      for(SysInt j = 0; j < (SysInt)literalsScratch.size(); ++j)
        assignment.push_back(literalsScratch[j]);
      return true;
    }
  }
  return false;
}

virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
  D_ASSERT(vars.size() > 0);
  if(v[0] < vars[0].initialMin())
    return false;

  if(v[0] > vars[0].initialMax())
    return false;

  const SysInt val_offset = checked_cast<SysInt>(v[0] - vars[0].initialMin());
  const vector<vector<pair<SysInt, DomainInt>>*>& tuplist = tuple_list->get_tl()[0][val_offset];

  for(SysInt i = 0; i < (SysInt)tuple_list->get_tl()[0][val_offset].size(); i++) {
    const vector<pair<SysInt, DomainInt>>& tup = *(tuplist[i]);

    SysInt supsize = tup.size();
    bool valid = true;

    for(SysInt j = 0; j < supsize; j++) {
      if(v[tup[j].first] != tup[j].second) {
        valid = false;
        break;
      }
    }

    if(valid)
      return true;
  }

  return false;
}

virtual ~CLASSNAME() {
  // printStructures();
  set<Support*> myset;

  /*
  for(SysInt i=0; i<(SysInt)vars.size(); i++) {
      cout << "     i " << i << " Initial Max " << vars[i].initialMax() <<
  endl ;
      SysInt numvals_i = vars[i].initialMax()-vars[i].initialMin()+1;
      for(SysInt j=0; j<numvals_i; j++) {
        cout << "     i j SupportListPerLit[var][val].next = " << i << " " << j
  << " " << supportListPerLit[i][j].next << endl ;
      }
  }
  */

  // Want to find all active support objects so we can delete them
  for(SysInt lit = 0; lit < numlits; lit++) {
    SupportCell* supCell = literalList[lit].supportCellList;

    // cout << "     destructor 2: sup*= " << sup << endl ;
    while(supCell != 0) {
      myset.insert(supCell->sup); // may get inserted multiple times but it's a set.
      supCell = supCell->next;
    }
  }

  // Go through supportFreeList

  while(supportFreeList != 0) {
    Support* sup = supportFreeList;
    supportFreeList = sup->nextFree;
    myset.insert(sup);
  }

  // Anything remaining on bracktrack stack
  for(SysInt i = 0; i < (SysInt)backtrack_stack.size(); i++) {
    if(backtrack_stack[i].sup != 0) {
      myset.insert(backtrack_stack[i].sup);
    }
  }

  typename set<Support*>::iterator it;
  for(it = myset.begin(); it != myset.end(); it++) {
    delete *it;
  }
}

Support* getFreeSupport() {
  // Either get a Support off the free list or make one.
  if(supportFreeList == 0) {
    return new Support();
  } else {
    Support* temp = supportFreeList;
    supportFreeList = supportFreeList->nextFree;
    return temp;
  }
}

void full_prop_init() {
  SysInt numvars = vars.size();

  // Initialise counters
  supports = 0;
  supportsPerVar.clear();
  supportsPerVar.resize(numvars, 0);

  {
    DomainInt litCounter = 0;
    numvals = 0; // only used now by tuple list stuff

    for(SysInt i = 0; i < numvars; i++) {

      firstLiteralPerVar[i] = checked_cast<SysInt>(litCounter);
      DomainInt numvals_i = vars[i].initialMax() - vars[i].initialMin() + 1;
      if(numvals_i > numvals)
        numvals = checked_cast<SysInt>(numvals_i);
      litCounter += numvals_i;
    }
    literalList.clear();
    literalList.resize(checked_cast<SysInt>(litCounter));
  }
  {
    SysInt litCounter = 0;
    for(SysInt i = 0; i < numvars; i++) {
      DomainInt thisvalmin = vars[i].initialMin();
      DomainInt numvals_i = vars[i].initialMax() - thisvalmin + 1;
      for(DomainInt j = 0; j < numvals_i; j++) {
        literalList[litCounter].var = i;
        literalList[litCounter].val = j + thisvalmin;
        literalList[litCounter].supportCellList = 0;
        litCounter++;
      }
    }

    numlits = litCounter;
  }
  zeroLits.clear();
  zeroLits.resize(numvars);
  for(SysInt i = 0; i < numvars; i++) {
    const SysInt numvals_i =
        checked_cast<SysInt>(vars[i].initialMax() - vars[i].initialMin() + 1);
    zeroLits[i].reserve(numvals_i); // reserve the maximum length.
    zeroLits[i].resize(0);
    SysInt thisvarstart = firstLiteralPerVar[i];
    for(SysInt j = 0; j < numvals_i; j++)
      zeroLits[i].push_back(j + thisvarstart);
  }
  inZeroLits.clear();
  inZeroLits.resize(numlits, true);

  // Lists (vectors) of literals/vars that have lost support.
  // Set this up to insist that everything needs to have support found for it on
  // full propagate.

  varsWithLostImplicitSupport.clear();
  varsWithLostImplicitSupport.reserve(vars.size());

  // Partition
  varsPerSupport.clear();
  varsPerSupport.resize(vars.size());
  varsPerSupInv.clear();
  varsPerSupInv.resize(vars.size());
  for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
    varsPerSupport[i] = i;
    varsPerSupInv[i] = i;
  }

  // Start with 1 cell in partition, for 0 supports.
  supportNumPtrs.clear();
  supportNumPtrs.resize(numlits + 1);
  supportNumPtrs[0] = 0;
  for(SysInt i = 1; i <= numlits; i++)
    supportNumPtrs[i] = vars.size();

  tuple_list_pos.clear();
  tuple_list_pos.resize(vars.size());
  for(SysInt var = 0; var < (SysInt)vars.size(); var++) {
    SysInt domsize =
        checked_cast<SysInt>(vars[var].initialMax() - vars[var].initialMin() + 1);
    tuple_list_pos[var].clear();
    tuple_list_pos[var].resize(domsize, 0);
  }
}

void init() {
  SysInt numvars = vars.size();

  data->validateShortTuples(numvars);

  // literalsScratch.reserve(numvars);

  literalsScratch.resize(0);

  // Register this with the backtracker.
  getState().getGenericBacktracker().add(this);

  // Initialise counters
  supports = 0;
  supportsPerVar.clear();
  supportsPerVar.resize(numvars, 0);

  firstLiteralPerVar.clear();
  firstLiteralPerVar.resize(numvars);

  {
    DomainInt litCounter = 0;
    numvals = 0; // only used now by tuple list stuff

    for(SysInt i = 0; i < numvars; i++) {

      firstLiteralPerVar[i] = checked_cast<SysInt>(litCounter);
      DomainInt numvals_i = vars[i].initialMax() - vars[i].initialMin() + 1;
      if(numvals_i > numvals)
        numvals = checked_cast<SysInt>(numvals_i);
      litCounter += numvals_i;
    }
    literalList.clear();
    literalList.resize(checked_cast<SysInt>(litCounter));
  }
  {
    SysInt litCounter = 0;
    for(SysInt i = 0; i < numvars; i++) {
      DomainInt thisvalmin = vars[i].initialMin();
      DomainInt numvals_i = vars[i].initialMax() - thisvalmin + 1;
      for(DomainInt j = 0; j < numvals_i; j++) {
        literalList[litCounter].var = i;
        literalList[litCounter].val = j + thisvalmin;
        literalList[litCounter].supportCellList = 0;
        litCounter++;
      }
    }

    numlits = litCounter;
  }

  zeroLits.clear();
  zeroLits.resize(numvars);
  for(SysInt i = 0; i < numvars; i++) {
    const SysInt numvals_i =
        checked_cast<SysInt>(vars[i].initialMax() - vars[i].initialMin() + 1);
    zeroLits[i].reserve(numvals_i); // reserve the maximum length.
    zeroLits[i].resize(0);
    SysInt thisvarstart = firstLiteralPerVar[i];
    for(SysInt j = 0; j < numvals_i; j++)
      zeroLits[i].push_back(j + thisvarstart);
  }
  inZeroLits.clear();
  inZeroLits.resize(numlits, true);

  // Lists (vectors) of literals/vars that have lost support.
  // Set this up to insist that everything needs to have support found for it on
  // full propagate.

  varsWithLostImplicitSupport.reserve(vars.size());

  // Partition
  varsPerSupport.resize(vars.size());
  varsPerSupInv.resize(vars.size());
  for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
    varsPerSupport[i] = i;
    varsPerSupInv[i] = i;
  }

  // Start with 1 cell in partition, for 0 supports.
  supportNumPtrs.resize(numlits + 1);
  supportNumPtrs[0] = 0;
  for(SysInt i = 1; i <= numlits; i++)
    supportNumPtrs[i] = vars.size();

  tuple_list_pos.resize(vars.size());
  for(SysInt var = 0; var < (SysInt)vars.size(); var++) {
    SysInt domsize =
        checked_cast<SysInt>(vars[var].initialMax() - vars[var].initialMin() + 1);
    tuple_list_pos[var].resize(domsize, 0);
  }

  tuple_list = data->getHaggisData(vars);
}

void printStructures() {
  cout << "PRINTING ALL DATA STRUCTURES" << endl;
  cout << "supports:" << supports << endl;
  cout << "supportsPerVar:" << supportsPerVar << endl;
  cout << "partition:" << endl;
  for(SysInt i = 0; i < (SysInt)supportNumPtrs.size() - 1; i++) {
    cout << "supports: " << i << "  vars: ";
    for(SysInt j = supportNumPtrs[i]; j < supportNumPtrs[i + 1]; j++) {
      cout << varsPerSupport[j] << ", ";
    }
    cout << endl;
    if(supportNumPtrs[i + 1] == vars.size())
      break;
  }
  cout << "zeroLits:" << zeroLits << endl;
  cout << "inZeroLits:" << inZeroLits << endl;
}

SysInt dynamicTriggerCount() {
  return literalList.size();
}

inline void updateCounters(SysInt lit) {

  SupportCell* supCellList = literalList[lit].supportCellList;

  litsWithLostExplicitSupport.resize(0);
  varsWithLostImplicitSupport.resize(0);

  while(supCellList != 0) {
    SupportCell* next = supCellList->next;
    deleteSupport(supCellList->sup);
    supCellList = next;
  }
}

inline void attach_trigger(SysInt var, DomainInt val, SysInt lit) {
  // P("Attach Trigger: " << i);
  moveTriggerInt(vars[var], lit, DomainRemoval, val); //, TO_Backtrack
}

inline void detach_trigger(SysInt lit) {
  // P("Detach Triggers");
  releaseTriggerInt(lit); // , TO_Backtrack
}

BOOL hasNoKnownSupport(SysInt var, SysInt lit) {
  //
  // Either implicitly supported or counter is non zero
  // Note that even if we have an explicit support which may be invalid, we can
  // return true
  // i.e. code does not guarantee that it has a valid support, only that it has
  // a support.
  // If we have no valid supports then (if algorithms are right) we will
  // eventually delete
  // the last known valid support and at that time start looking for a new one.

  D_ASSERT(var == literalList[lit].var);

  return supportsPerVar[var] == supports && (literalList[lit].supportCellList == 0);
}

void partition_swap(SysInt xi, SysInt xj) {
  if(xi != xj) {
    varsPerSupport[varsPerSupInv[xj]] = xi;
    varsPerSupport[varsPerSupInv[xi]] = xj;
    SysInt temp = varsPerSupInv[xi];
    varsPerSupInv[xi] = varsPerSupInv[xj];
    varsPerSupInv[xj] = temp;
  }
}

////////////////////////////////////////////////////////////////////////////
//
//  Table of short supports passed in.

#define ADDTOASSIGNMENT(var, val) literalsScratch.push_back(make_pair(var, val));

// bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var,
// DomainInt val) {
template <bool keepassigned>
bool findNewSupport(SysInt var, DomainInt val) {
  D_ASSERT(tuple_list->get_tl().size() == vars.size());
  const SysInt val_offset = checked_cast<SysInt>(val - vars[var].initialMin());
  const vector<vector<pair<SysInt, DomainInt>>*>& tuplist = tuple_list->get_tl()[var][val_offset];

  SysInt listsize = tuplist.size();
  for(SysInt i = tuple_list_pos[var][val_offset]; i < listsize; i++) {
    vector<pair<SysInt, DomainInt>>& tup = *(tuplist[i]);

    SysInt supsize = tup.size();
    bool valid = true;

    for(SysInt j = 0; j < supsize; j++) {
      if(!vars[tup[j].first].inDomain(tup[j].second)) {
        valid = false;
        break;
      }
    }

    if(valid) {
      for(SysInt j = 0; j < supsize; j++) {
        ADDTOASSIGNMENT(tup[j].first, tup[j].second); // assignment.push_back(tuplist[i][j]);
      }
      tuple_list_pos[var][val_offset] = i;
      return true;
    }
  }

  for(SysInt i = 0; i < tuple_list_pos[var][val_offset]; i++) {
    vector<pair<SysInt, DomainInt>>& tup = *(tuplist[i]);

    SysInt supsize = tup.size();
    bool valid = true;

    for(SysInt j = 0; j < supsize; j++) {
      if(!vars[tup[j].first].inDomain(tup[j].second)) {
        valid = false;
        break;
      }
    }

    if(valid) {
      for(SysInt j = 0; j < supsize; j++) {
        ADDTOASSIGNMENT(tup[j].first, tup[j].second); // assignment.push_back(tuplist[i][j]);
      }
      tuple_list_pos[var][val_offset] = i;
      return true;
    }
  }
  return false;
}
