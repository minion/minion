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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/** @help constraints;shortctuplestr2 Description

This constraint extends the ShortSTR2 algorithm to support short c-tuples
(that is, short tuples which contain can contain more than one domain value
per constraint).

*/

/** @help constraints;shortctuplestr2 Example

Input format is similar to that used by other short tuple constraints,
such as haggisgac or shortstr2. Refer to the haggisgac and
shorttuplelist pages for more information.

The important change is that more than one literal may be given for each
variable. Variables which are not mentioned are assumed to be allowed
to take any value

Example:

**SHORTTUPLELIST**
mycon 4
[(0,0),(0,1),(3,0)]
[(1,0),(1,2),(3,0)]
[(2,0),(3,0),(3,1)]
[(0,1),(1,1),(2,1),(3,1)]

**CONSTRAINTS**
shortctuplestr2([x1,x2,x3,x4], mycon)

*/

/** @help constraints;shortcuplestr2 Notes
This constraint enforces generalized arc consistency.
*/

/** @help constraints;shortctuplestr2 References
help input shorttuplelist
help constraints table
help constraints negativetable
help constraints haggisgac
help constraints haggisgac-stable
help constraints shortstr2
*/

#ifndef CONSTRAINT_CTUPLESTR2_H
#define CONSTRAINT_CTUPLESTR2_H

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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#include "constraint_checkassign.h"
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <vector>

using namespace std;

struct arrayset_boundscheck {
  vector<SysInt> vals;
  vector<SysInt> vals_pos;
  DomainInt size;
  DomainInt minval;

  void initialise(DomainInt low, DomainInt high) {
    minval = low;
    vals_pos.resize(checked_cast<SysInt>(high - low + 1));
    vals.resize(checked_cast<SysInt>(high - low + 1));
    for(SysInt i = 0; i < checked_cast<SysInt>(high - low + 1); i++) {
      vals[i] = checked_cast<SysInt>(i + low);
      vals_pos[i] = i;
    }
    size = 0;
  }

  void clear() {
    size = 0;
  }

  bool out_of_bounds(DomainInt val) {
    DomainInt pos = val - minval;
    return (pos < 0 || pos >= vals_pos.size());
  }

  bool in(DomainInt val) {
    if(out_of_bounds(val))
      return false;
    return vals_pos[checked_cast<SysInt>(val - minval)] < size;
  }

private:
  // This method looks a bit messy, due to stupid C++ optimisers not being
  // clever enough to realise various things don't alias, and this method
  // being called as much as it is.
  void unsafe_insert(DomainInt val) {
    D_ASSERT(!out_of_bounds(val));
    D_ASSERT(!in(val));
    const SysInt minval_cpy = checked_cast<SysInt>(minval);
    const SysInt validx = checked_cast<SysInt>(val - minval_cpy);
    const SysInt size_cpy = checked_cast<SysInt>(size);
    const SysInt swapval = vals[size_cpy];
    const SysInt vpvx = vals_pos[validx];
    vals[vpvx] = swapval;
    vals[size_cpy] = checked_cast<SysInt>(val);

    vals_pos[checked_cast<SysInt>(swapval - minval_cpy)] = vpvx;
    vals_pos[validx] = size_cpy;

    size++;
  }

public:
  void insert(DomainInt val) {
    if(!in(val)) {
      unsafe_insert(val);
    }
  }

private:
  void unsafe_remove(DomainInt val) {
    D_ASSERT(!out_of_bounds(val));
    // swap to posiition size-1 then reduce size
    D_ASSERT(in(val));
    const SysInt validx = checked_cast<SysInt>(val - minval);
    const SysInt swapval = vals[checked_cast<SysInt>(size - 1)];
    vals[vals_pos[validx]] = swapval;
    vals[checked_cast<SysInt>(size - 1)] = checked_cast<SysInt>(val);

    vals_pos[checked_cast<SysInt>(swapval - minval)] = vals_pos[validx];
    vals_pos[validx] = checked_cast<SysInt>(size - 1);

    size--;
  }

public:
  void remove(DomainInt val) {
    if(in(val)) {
      unsafe_remove(val);
    }
  }

  void fill() {
    size = vals.size();
  }
};

struct ReversibleArrayset_boundscheck {
  // Only allows deletion.
  vector<SysInt> vals;
  vector<SysInt> vals_pos;
  ReversibleInt size;
  SysInt minval;

  ReversibleArrayset_boundscheck() : size() {}

  void initialise(DomainInt low_, DomainInt high_) {
    const SysInt low = checked_cast<SysInt>(low_);
    const SysInt high = checked_cast<SysInt>(high_);
    minval = low;
    vals_pos.resize(high - low + 1);
    vals.resize(high - low + 1);
    for(SysInt i = 0; i < high - low + 1; i++) {
      vals[i] = i + low;
      vals_pos[i] = i;
    }
    size = vals.size();
  }

  bool out_of_bounds(DomainInt val) {
    DomainInt pos = val - minval;
    return (pos < 0 || pos >= vals_pos.size());
  }

  bool in(DomainInt val) {
    if(out_of_bounds(val))
      return false;
    return vals_pos[checked_cast<SysInt>(val - minval)] < size;
  }

  void remove(DomainInt val) {
    if(out_of_bounds(val))
      return;
    // swap to posiition size-1 then reduce size
    if(in(val)) {
      const SysInt validx = checked_cast<SysInt>(val - minval);
      const SysInt swapval = vals[size - 1];
      vals[vals_pos[validx]] = swapval;
      vals[size - 1] = checked_cast<SysInt>(val);

      vals_pos[swapval - minval] = vals_pos[validx];
      vals_pos[validx] = size - 1;

      size = size - 1;
    }
  }
};

struct CTupleSTRData {
  vector<vector<vector<DomainInt>>> tuples;
  vector<vector<pair<SysInt, DomainInt>>> compressed_tuples;

  CTupleSTRData(ShortTupleList* _tuples, size_t varsize) {
    _tuples->validateShortCTuples(varsize);
    compressed_tuples = *(_tuples->tuplePtr());

    for(SysInt i = 0; i < (SysInt)compressed_tuples.size(); ++i) {
      vector<vector<DomainInt>> temp(varsize);
      for(SysInt j = 0; j < (SysInt)compressed_tuples[i].size(); ++j) {
        temp[compressed_tuples[i][j].first].push_back(compressed_tuples[i][j].second);
      }
      for(SysInt i = 0; i < varsize; ++i) {
        std::sort(temp[i].begin(), temp[i].end());
      }
      tuples.push_back(temp);
    }
  }
};

template <typename VarArray, bool UseShort>
struct CTupleSTR : public AbstractConstraint {
  virtual string constraint_name() {
    return "shortctuplestr2";
  }

  //    CONSTRAINT_ARG_LIST2(vars, tupleList);

  virtual string full_output_name() {
    return ConOutput::print_con(constraint_name(), vars, shortTupleList);
  }

  ShortTupleList* shortTupleList;
  TupleList* longTupleList;

  VarArray vars;

  bool constraint_locked;

  vector<SysInt> tupindices;

  ReversibleInt limit; // In tupindices, indices less than limit are not known
                       // to be invalid.

  CTupleSTRData* sct;

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
      gacvalues[i].initialise(vars[i].getInitialMin(), vars[i].getInitialMax());
    }

    std::random_shuffle(tupindices.begin(), tupindices.end());
  }

  CTupleSTR(const VarArray& _var_array, ShortTupleList* _tuples)
      : shortTupleList(_tuples),
        longTupleList(0),
        vars(_var_array),
        constraint_locked(false),
        limit(),
        sct(new CTupleSTRData(_tuples, _var_array.size()))
  //, ssup_permanent()
  {
    CHECK(UseShort, "Internal error in ShortSTR2");
    init();
  }

  virtual SysInt dynamic_trigger_count() {
    return vars.size();
  }

  void setup_triggers() {
    for(SysInt i = 0; i < vars.size(); ++i) {
      moveTriggerInt(vars[i], i, DomainChanged);
    }
  }

  virtual void full_propagate() {
    setup_triggers();
    limit = sct->tuples.size();

    // pretend all variables have changed.
    for(SysInt i = 0; i < (SysInt)vars.size(); i++)
      sval.insert(i);

    do_prop();
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> ret;
    ret.reserve(vars.size());
    for(unsigned i = 0; i < vars.size(); ++i)
      ret.push_back(vars[i]);
    return ret;
  }

  virtual bool check_assignment(DomainInt* v, SysInt v_size) {
    const vector<set<DomainInt>>& doms = shortTupleList->getInitialDomains();
    if(doms.size() > 0) {
      for(SysInt i = 0; i < v_size; ++i) {
        if(doms[i].count(v[i]) == 0)
          return false;
      }
    }

    SysInt varsize = vars.size();
    for(SysInt i = 0; i < (SysInt)sct->tuples.size(); ++i) {
      bool sat = true;
      D_ASSERT(sct->tuples[i].size() == varsize);
      for(SysInt j = 0; j < varsize; ++j) {
        if(sct->tuples[i][j].size() > 0) {
          if(!std::binary_search(sct->tuples[i][j].begin(), sct->tuples[i][j].end(), v[j])) {
            sat = false;
            break;
          }
        }
      }

      if(sat)
        return true;
    }

    return false;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(SysInt i = 0; i < (SysInt)sct->tuples.size(); ++i) {
      bool sat = true;
      assignment.clear();
      for(SysInt j = 0; j < (SysInt)sct->tuples[i].size(); ++j) {
        if(sct->tuples[i][j].size() > 0) {
          // check if any value is supported
          bool found = false;
          for(SysInt k = 0; k < (SysInt)sct->tuples[i][j].size(); ++k) {
            if(vars[j].inDomain(sct->tuples[i][j][k])) {
              assignment.push_back(std::make_pair(j, sct->tuples[i][j][k]));
              found = true;
              break;
            }
          }

          if(found == false) {
            sat = false;
            break;
          }
        }
      }

      if(sat) {
        return true;
      }
    }
    assignment.clear();
    return false;
  }

  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }

  virtual void propagateDynInt(SysInt prop_var, DomainDelta) {
    sval.insert(prop_var);

    if(!constraint_locked) {
      constraint_locked = true;
      getQueue().pushSpecialTrigger(this);
    }
  }

  virtual void special_unlock() {
    constraint_locked = false;
    sval.clear();
  }

  virtual void special_check() {
    constraint_locked = false;
    D_ASSERT(!getState().isFailed());
    do_prop();
  }

  // S_sup is the set of unset (by the search procedure) variables with
  // at least one unsupported val.
  // Iterate only on S_Sup in the main loops looking for support.
  // Unfortunately can't do this exactly as in STR2 paper.
  arrayset_boundscheck ssup;

  // ReversibleArrayset ssup_permanent;  // when a var is assigned and after
  // str2 has been run, it is removed from here.

  // S_val is the set of "unassigned" vars whose domain has been reduced since
  // previous call.
  // Unassigned here I think means not assigned by the search procedure.
  // Also contains the last assigned var (i.e. last assigned by the search
  // procedure)
  // if it belongs to the scope of the constraint.

  // Here interpreted as the set of variables that triggered this call.
  arrayset_boundscheck sval;

  // lastSize array dropped. Only need to keep a list of the triggering vars.

  vector<arrayset_boundscheck> gacvalues;

  bool validTuple(SysInt i) {
    SysInt index = tupindices[i];
    const vector<vector<DomainInt>>& tau = sct->tuples[index];
    D_ASSERT(tau.size() == vars.size());
    // cout << "Checking Tuple " << tau << "\n";
    for(SysInt j = 0; j < tau.size(); j++) {
      const std::vector<DomainInt>& tv = tau[j];
      // cout << "Checking index " << j << "\n";
      if(tv.size() > 0) {
        bool found = false;
        for(SysInt k = 0; k < (SysInt)tv.size(); ++k) {
          if(vars[j].inDomain(tv[k])) {
            found = true;
            break;
          }
        }
        if(found == false) {
          // cout << "pruned\n";
          return false;
        }
      }
    }
    // cout << "fine\n";
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
            // cout << "Considering var " << ctf << "\n";
            ssup.insert(ctf);
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

    vector<vector<vector<DomainInt>>>::iterator tup_start = sct->tuples.begin();

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
      const vector<vector<DomainInt>>& tau = tup_start[index];
      // do stuff
      for(SysInt j = 0; j < ssup.size; j++) {
        const SysInt var = ssup.vals[j];

        if(UseShort && tau[var].size() == 0) {
          if(ssup.in(var)) {
            // cout << "ShortSkip " << var << "\n";
            ssup.remove(var);
            j--;
          }
          // if(vars[var].isAssigned()) ssup_permanent.remove(var);
        } else {

          for(SysInt k = 0; k < tau[var].size(); ++k) {
            // cout << "Considering " << var << ":" << tau[var][k] << "\n";
            if(!gacvalues[var].in(tau[var][k]) && vars[var].inDomain(tau[var][k])) {
              gacvalues[var].insert(tau[var][k]);

              if(gacvalues[var].size == vars[var].getDomSize() && ssup.in(var)) {
                // cout << "Filled var " << var;
                ssup.remove(var);
                j--;
                // if(vars[var].isAssigned()) ssup_permanent.remove(var);
              }
            }
          }
        }
      }
      i++;
    }

    // Prune the domains.
    for(SysInt j = 0; j < ssup.size; j++) {
      SysInt var = ssup.vals[j];
      for(DomainInt val = vars[var].getMin(); val <= vars[var].getMax(); val++) {
        if(!gacvalues[var].in(val)) {
          vars[var].removeFromDomain(val);
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
AbstractConstraint* BuildCT_SHORTSTR_CTUPLE(const T& t1, ConstraintBlob& b) {
  return new CTupleSTR<T, true>(t1, b.short_tuples);
}

/* JSON
  { "type": "constraint",
    "name": "shortctuplestr2",
    "internal_name": "CT_SHORTSTR_CTUPLE",
    "args": [ "read_list", "read_short_tuples" ]
  }
  */

#endif
