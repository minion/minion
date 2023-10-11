// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0



/** @help constraints;shortstr2 Example

Input format is exactly the same as haggisgac. Refer to the haggisgac and
shorttuplelist pages for more information.

Example:

**SHORTTUPLELIST**
mycon 4
[(0,0),(3,0)]
[(1,0),(3,0)]
[(2,0),(3,0)]
[(0,1),(1,1),(2,1),(3,1)]

**CONSTRAINTS**
shortstr2([x1,x2,x3,x4], mycon)

*/











#ifndef CONSTRAINT_STR2_H
#define CONSTRAINT_STR2_H

// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0
#include "constraint_checkassign.h"
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <vector>

using namespace std;

#include "arrayset.h"

struct ReversibleArrayset {
  // Only allows deletion.
  vector<SysInt> vals;
  vector<SysInt> valsPos;
  ReversibleInt size;
  SysInt minval;

  ReversibleArrayset() : size() {}

  void initialise(DomainInt low_, DomainInt high_) {
    const SysInt low = checked_cast<SysInt>(low_);
    const SysInt high = checked_cast<SysInt>(high_);
    minval = low;
    valsPos.resize(high - low + 1);
    vals.resize(high - low + 1);
    for(SysInt i = 0; i < high - low + 1; i++) {
      vals[i] = i + low;
      valsPos[i] = i;
    }
    size = vals.size();
  }

  bool in(DomainInt val) {
    return valsPos[checked_cast<SysInt>(val - minval)] < size;
  }

  void remove(DomainInt val) {
    // swap to posiition size-1 then reduce size
    if(in(val)) {
      const SysInt validx = checked_cast<SysInt>(val - minval);
      const SysInt swapval = vals[size - 1];
      vals[valsPos[validx]] = swapval;
      vals[size - 1] = checked_cast<SysInt>(val);

      valsPos[swapval - minval] = valsPos[validx];
      valsPos[validx] = size - 1;

      size = size - 1;
    }
  }
};

struct STRData {
  vector<vector<DomainInt>> tuples;
  vector<vector<pair<SysInt, DomainInt>>> compressed_tuples;

  STRData(std::shared_ptr<ShortTupleList> _tuples, size_t varsize) {
    _tuples->validateShortTuples(varsize);
    compressed_tuples = *(_tuples->tuplePtr());

    for(SysInt i = 0; i < (SysInt)compressed_tuples.size(); ++i) {
      vector<DomainInt> temp(varsize, DomainInt_Skip);
      for(SysInt j = 0; j < (SysInt)compressed_tuples[i].size(); ++j) {
        temp[compressed_tuples[i][j].first] = compressed_tuples[i][j].second;
      }
      tuples.push_back(temp);
    }
  }

  STRData(std::shared_ptr<TupleList> _tuples, size_t varsize) {
    DomainInt tupleCount = _tuples->size();
    for(SysInt i = 0; i < tupleCount; ++i) {
      vector<DomainInt> t = _tuples->getVector(i);
      vector<pair<SysInt, DomainInt>> comp;
      for(int j = 0; j < (SysInt)t.size(); ++j)
        comp.push_back(std::make_pair(j, t[j]));
      tuples.push_back(t);
      compressed_tuples.push_back(comp);
    }
  }
};

template <typename VarArray, bool UseShort>
struct STR : public AbstractConstraint {
  virtual string constraintName() {
    if(UseShort)
      return "shortstr2";
    else
      return "str2plus";
  }

  //    CONSTRAINT_ARG_LIST2(vars, tupleList);

  virtual string fullOutputName() {
    if(UseShort)
      return ConOutput::printCon(constraintName(), vars, shortTupleList);
    else
      return ConOutput::printCon(constraintName(), vars, longTupleList);
  }

  std::shared_ptr<ShortTupleList> shortTupleList;
  std::shared_ptr<TupleList> longTupleList;

  VarArray vars;

  bool constraintLocked;

  vector<SysInt> tupindices;

  ReversibleInt limit; // In tupindices, indices less than limit are not known
                       // to be invalid.

  std::unique_ptr<STRData> sct;

  void init() {
    if((SysInt)sct->tuples.size() > 0) {
      CHECK(sct->tuples[0].size() == vars.size(),
            "Cannot use same table for two constraints with different numbers "
            "of variables!");
    }
    tupindices.resize(sct->tuples.size());
    for(SysInt i = 0; i < (SysInt)sct->tuples.size(); i++) {
      tupindices[i] = i;
    }

    ssup.initialise(0, (SysInt)vars.size() - 1);
    sval.initialise(0, (SysInt)vars.size() - 1);

    // ssup_permanent.initialise(0, (SysInt)vars.size()-1);

    gacvalues.resize(vars.size());
    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      gacvalues[i].initialise(vars[i].initialMin(), vars[i].initialMax());
    }

    std::random_shuffle(tupindices.begin(), tupindices.end());
  }

  STR(const VarArray& _varArray, std::shared_ptr<ShortTupleList> _tuples)
      : shortTupleList(_tuples),
        longTupleList(0),
        vars(_varArray),
        constraintLocked(false),
        limit(),
        sct(new STRData(_tuples, _varArray.size()))
  //, ssup_permanent()
  {
    CHECK(UseShort, "Internal error in ShortSTR2");
    init();
  }

  STR(const VarArray& _varArray, std::shared_ptr<TupleList> _tuples)
      : shortTupleList(0),
        longTupleList(_tuples),
        vars(_varArray),
        constraintLocked(false),
        limit(),
        sct(new STRData(_tuples, _varArray.size()))
  //, ssup_permanent()
  {
    CHECK(!UseShort, "Internal error in str2plus");
    init();
  }

  virtual SysInt dynamicTriggerCount() {
    return vars.size();
  }

  void setupTriggers() {
    for(SysInt i = 0; i < vars.size(); ++i) {
      moveTriggerInt(vars[i], i, DomainChanged);
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    limit = sct->tuples.size();

    // pretend all variables have changed.
    for(SysInt i = 0; i < (SysInt)vars.size(); i++)
      sval.insert(i);

    do_prop();
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> ret;
    ret.reserve(vars.size());
    for(unsigned i = 0; i < vars.size(); ++i)
      ret.push_back(vars[i]);
    return ret;
  }

  virtual bool checkAssignment(DomainInt* v, SysInt vSize) {
    if(UseShort) {
      const vector<set<DomainInt>>& doms = shortTupleList->initialDomains();
      if(doms.size() > 0) {
        for(SysInt i = 0; i < vSize; ++i) {
          if(doms[i].count(v[i]) == 0)
            return false;
        }
      }
    }

    for(SysInt i = 0; i < (SysInt)sct->compressed_tuples.size(); ++i) {
      bool sat = true;
      for(SysInt j = 0; j < (SysInt)sct->compressed_tuples[i].size(); ++j) {
        if(v[sct->compressed_tuples[i][j].first] != sct->compressed_tuples[i][j].second) {
          sat = false;
          break;
        }
      }

      if(sat)
        return true;
    }

    return false;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {

    for(SysInt i = 0; i < (SysInt)sct->compressed_tuples.size(); ++i) {
      bool sat = true;
      for(SysInt j = 0; j < (SysInt)sct->compressed_tuples[i].size(); ++j) {
        if(!vars[sct->compressed_tuples[i][j].first].inDomain(
               sct->compressed_tuples[i][j].second)) {
          sat = false;
          break;
        }
      }

      if(sat) {
        for(SysInt j = 0; j < (SysInt)sct->compressed_tuples[i].size(); ++j)
          assignment.push_back(sct->compressed_tuples[i][j]);
        return true;
      }
    }

    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }

  virtual void propagateDynInt(SysInt prop_var, DomainDelta) {
    sval.insert(prop_var);

    if(!constraintLocked) {
      constraintLocked = true;
      getQueue().pushSpecialTrigger(this);
    }
  }

  virtual void specialUnlock() {
    constraintLocked = false;
    sval.clear();
  }

  virtual void specialCheck() {
    constraintLocked = false;
    D_ASSERT(!getState().isFailed());
    do_prop();
  }

  // S_sup is the set of unset (by the search procedure) variables with
  // at least one unsupported val.
  // Iterate only on S_Sup in the main loops looking for support.
  // Unfortunately can't do this exactly as in STR2 paper.
  arrayset ssup;

  // ReversibleArrayset ssup_permanent;  // when a var is assigned and after
  // str2 has been run, it is removed from here.

  // S_val is the set of "unassigned" vars whose domain has been reduced since
  // previous call.
  // Unassigned here I think means not assigned by the search procedure.
  // Also contains the last assigned var (i.e. last assigned by the search
  // procedure)
  // if it belongs to the scope of the constraint.

  // Here interpreted as the set of variables that triggered this call.
  arrayset sval;

  // lastSize array dropped. Only need to keep a list of the triggering vars.

  vector<arrayset> gacvalues;

  bool validTuple(SysInt i) {
    SysInt index = tupindices[i];
    const vector<DomainInt>& tau = sct->tuples[index];

    for(SysInt j = 0; j < sval.size; j++) {
      const SysInt var = sval.vals[j];
      if(UseShort) {
        const DomainInt tv = tau[var];
        if((tv != DomainInt_Skip) && !vars[var].inDomain(tv)) {
          return false;
        }
      } else {
        D_ASSERT(tau[var] != DomainInt_Skip);
        if(!vars[var].inDomain(tau[var])) {
          return false;
        }
      }
    }

    return true;
  }

  void do_prop() {
    const SysInt numvars = vars.size();

    // Basic impl of ssup for now.
    // For 'removing assigned vars' optimization, need them to be both
    // assigned and to have done the table reduction after assignment!

    // Actually this thing below is OK: as soon as we find a valid tuple,
    // any assigned vars will be removed from ssup.

    // ssup.fill();

    // copy ssup_permanent into ssup.
    // ssup.clear();
    // for(int j=0; j<ssup_permanent.size; j++)
    // ssup.insert(ssup_permanent.vals[j]);

    if(UseShort) {
      ssup.clear();

      while(limit > 0) {
        const SysInt index = tupindices[0];
        // check validity
        bool isvalid = validTuple(0);

        if(isvalid) {
          const vector<pair<SysInt, DomainInt>>& compressed_tau = sct->compressed_tuples[index];
          for(SysInt t = 0; t < (SysInt)compressed_tau.size(); ++t) {
            const SysInt ctf = compressed_tau[t].first;
            ssup.unsafe_insert(ctf);
            gacvalues[ctf].clear();
          }

          break;
        } else {
          removeTuple(0);
        }
      }

      if(limit == 0) {
        // We found no valid tuples!
        getState().setFailed(true);
        return;
      }
    } else {
      for(SysInt t = 0; t < numvars; t++)
        gacvalues[t].clear();
      ssup.fill();
    }

    vector<vector<DomainInt>>::iterator tupStart = sct->tuples.begin();

    // We dealt with the first tuple, if we are in 'Short' mode.
    SysInt i = UseShort ? 1 : 0;

    while(i < limit) {
      // check validity
      if(!validTuple(i))
        removeTuple(i);
      else
        i++;
    }

    i = 0;
    SysInt lim_cpy = checked_cast<SysInt>(limit);
    while(i < lim_cpy && ssup.size > 0) {
      const SysInt index = tupindices[i];
      const vector<DomainInt>& tau = tupStart[index];
      // do stuff
      for(SysInt j = 0; j < ssup.size; j++) {
        const SysInt var = ssup.vals[j];

        if(UseShort && tau[var] == DomainInt_Skip) {
          ssup.unsafe_remove(var);
          j--;
          // if(vars[var].isAssigned()) ssup_permanent.remove(var);
        } else if(!gacvalues[var].in(tau[var])) {
          gacvalues[var].unsafe_insert(tau[var]);

          if(gacvalues[var].size == vars[var].domSize()) {
            ssup.unsafe_remove(var);
            j--;
            // if(vars[var].isAssigned()) ssup_permanent.remove(var);
          }
        }
      }
      i++;
    }

    // Prune the domains.
    for(SysInt j = 0; j < ssup.size; j++) {
      SysInt var = ssup.vals[j];
      if(vars[var].isBound()) {
        DomainInt new_min = vars[var].max() + 1;
        for(DomainInt val = vars[var].min(); val <= vars[var].max(); val++) {
          if(gacvalues[var].in(val)) {
            new_min = val;
            break;
          }
        }
        DomainInt new_max = vars[var].min() - 1;
        for(DomainInt val = vars[var].max(); val >= vars[var].min(); val--) {
          if(gacvalues[var].in(val)) {
            new_max = val;
            break;
          }
        }

        vars[var].setMin(new_min);
        vars[var].setMax(new_max);
      }
      else {
        for(DomainInt val = vars[var].min(); val <= vars[var].max(); val++) {
          if(!gacvalues[var].in(val)) {
            vars[var].removeFromDomain(val);
          }
        }
      }
    }

    sval.clear();
  }

  inline void removeTuple(SysInt i) {
    // Swap to end
    D_ASSERT(i < limit);
    SysInt tmp = tupindices[limit - 1];
    tupindices[limit - 1] = tupindices[i];
    tupindices[i] = tmp;
    limit = limit - 1;
  }
};

template <typename T>
AbstractConstraint* BuildCT_SHORTSTR(const T& t1, ConstraintBlob& b) {
  return new STR<T, true>(t1, b.shortTuples);
}

/* JSON
  { "type": "constraint",
    "name": "shortstr2",
    "internal_name": "CT_SHORTSTR",
    "args": [ "read_list", "read_short_tuples" ]
  }
  */

template <typename T>
AbstractConstraint* BuildCT_STR(const T& t1, ConstraintBlob& b) {
  return new STR<T, false>(t1, b.tuples);
}

/* JSON
  { "type": "constraint",
    "name": "str2plus",
    "internal_name": "CT_STR",
    "args": [ "read_list", "read_tuples" ]
  }
  */

#endif
