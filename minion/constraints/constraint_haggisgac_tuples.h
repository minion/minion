// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _HAGGIS_GAC_TUPLES_CHUDSCDFDSAFDSA
#define _HAGGIS_GAC_TUPLES_CHUDSCDFDSAFDSA

struct SupportDeref {
  template <typename T>
  bool operator()(const T& lhs, const T& rhs) {
    return *lhs < *rhs;
  }
};

struct HaggisGACTuples {
  typedef vector<vector<vector<vector<pair<SysInt, DomainInt>>*>>> tuple_list_type;

  tuple_list_type tuple_list_cpy;

  const tuple_list_type& getTL() const {
    return tuple_list_cpy;
  }

  template <typename Vars, typename Data>
  HaggisGACTuples(const Vars& vars, Data data) {
    // Read in the short supports.
    vector<vector<pair<SysInt, DomainInt>>*> shortsupports;

    const vector<vector<pair<SysInt, DomainInt>>>& tupleRef = (*data->tuplePtr());

    for(SysInt i = 0; i < (SysInt)tupleRef.size(); i++) {
      shortsupports.push_back(new vector<pair<SysInt, DomainInt>>(tupleRef[i]));
    }

    // Sort it. Might not work when it's pointers.
    for(SysInt i = 0; i < (SysInt)shortsupports.size(); i++) {
      sort(shortsupports[i]->begin(), shortsupports[i]->end());
    }
    sort(shortsupports.begin(), shortsupports.end(), SupportDeref());

    tuple_list_cpy.resize(vars.size());
    for(SysInt var = 0; var < (SysInt)vars.size(); var++) {
      SysInt domsize =
          checked_cast<SysInt>(vars[var].initialMax() - vars[var].initialMin() + 1);
      tuple_list_cpy[var].resize(domsize);

      for(DomainInt val = vars[var].initialMin(); val <= vars[var].initialMax(); val++) {
        // get short supports relevant to var,val.
        for(SysInt i = 0; i < (SysInt)shortsupports.size(); i++) {
          bool varin = false;
          bool valmatches = true;

          vector<pair<SysInt, DomainInt>>& shortsup = *(shortsupports[i]);

          for(SysInt j = 0; j < (SysInt)shortsup.size(); j++) {
            if(shortsup[j].first == var) {
              varin = true;
              if(shortsup[j].second != val) {
                valmatches = false;
              }
            }
          }

          if(!varin || valmatches) {
            // If the support doesn't include the var, or it
            // does include var,val then add it to the list.
            tuple_list_cpy[var][checked_cast<SysInt>(val - vars[var].initialMin())].push_back(
                shortsupports[i]);
          }
        }
      }
    }
  }
};

template <typename Vars>
inline HaggisGACTuples* ShortTupleList::getHaggisData(const Vars& vars) {
  vector<pair<DomainInt, DomainInt>> doms;

  for(SysInt i = 0; i < (SysInt)vars.size(); ++i)
    doms.push_back(std::make_pair(vars[i].initialMin(), vars[i].initialMax()));

  if(hgt.count(doms) == 0) {
    hgt[doms] = new HaggisGACTuples(vars, this);
  }

  return hgt[doms];
}

#endif
