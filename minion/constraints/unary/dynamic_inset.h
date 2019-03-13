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

/** @help constraints;w-inset Description
The constraint w-inset(x, [a1,...,an]) ensures that x belongs to the set
{a1,..,an}.
*/

/** @help constraints;w-inset References
  See also

  help constraints w-notinset
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_INSET_H
#define CONSTRAINT_DYNAMIC_UNARY_INSET_H

// Checks if a variable is in a fixed set.
template <typename Var>
struct WatchInSetConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-inset";
  }

  CONSTRAINT_ARG_LIST2(var, vals);

  Var var;

  vector<DomainInt> vals;

  template <typename T>
  WatchInSetConstraint(const Var& _var, const T& _vals)
      : var(_var), vals(_vals.begin(), _vals.end()) {
    stable_sort(vals.begin(), vals.end());
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  } // Only uses one!

  virtual void fullPropagate() {
    if(vals.empty()) {
      getState().setFailed(true);
      return;
    }
    var.setMin(vals.front());
    var.setMax(vals.back());

    if(var.isBound()) {
      // May as well pass DomainRemoval
      moveTriggerInt(var, 0, DomainChanged);
      propagateDynInt(0, DomainDelta::empty());
    } else {
      for(SysInt i = 0; i < (SysInt)vals.size() - 1; ++i)
        for(DomainInt pos = vals[i] + 1; pos < vals[i + 1]; ++pos)
          var.removeFromDomain(pos);
    }
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInSet);
    // If we are in here, we have a bounds variable.
    D_ASSERT(var.isBound());
    // This is basically lifted from "sparse SysInt bound vars"
    vector<DomainInt>::iterator it_low = std::lower_bound(vals.begin(), vals.end(), var.min());
    if(it_low == vals.end()) {
      getState().setFailed(true);
    } else {
      var.setMin(*it_low);
    }

    vector<DomainInt>::iterator it_high = std::lower_bound(vals.begin(), vals.end(), var.max());
    if(it_high == vals.end()) {
      var.setMax(*(it_high - 1)); // Wasn't this already done in fullPropagate?
      return;
    }

    if(*it_high == var.max())
      return;

    if(it_high == vals.begin()) {
      getState().setFailed(true);
      return;
    }

    var.setMax(*(it_high - 1));
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return binary_search(vals.begin(), vals.end(), v[0]);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    /// TODO: Make faster
    for(SysInt i = 0; i < (SysInt)vals.size(); ++i) {
      if(var.inDomain(vals[i])) {
        assignment.push_back(make_pair(0, vals[i]));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint();
};

// To get reverseConstraint
#include "dynamic_notinset.h"

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_INSET(const VarArray1& _varArray_1, const ConstraintBlob& b) {
  return new WatchInSetConstraint<typename VarArray1::value_type>(_varArray_1[0], b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-inset",
    "internal_name": "CT_WATCHED_INSET",
    "args": [ "read_var", "read_constant_list" ]
  }
*/
#endif
