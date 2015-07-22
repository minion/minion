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

/** @help constraints;w-notinset Description
The constraint w-notinset(x, [a1,...,an]) ensures that x does not belong to the
set {a1,..,an}.
*/

/** @help constraints;w-notinset References
  See also

  help constraints w-inset
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_NOTINSET_H
#define CONSTRAINT_DYNAMIC_UNARY_NOTINSET_H

#include "dynamic_inset.h"

// Checks if a variable is not in a fixed set.
template <typename Var>
struct WatchNotInSetConstraint : public AbstractConstraint {
  virtual string constraint_name() { return "w-notinset"; }

  CONSTRAINT_ARG_LIST2(var, vals);

  Var var;

  vector<DomainInt> vals;

  template <typename T>
  WatchNotInSetConstraint(const Var &_var, const T &_vals)
      : var(_var), vals(_vals.begin(), _vals.end()) {
    stable_sort(vals.begin(), vals.end());
  }

  virtual SysInt dynamic_trigger_count() { return 2; }

  virtual void full_propagate() {
    if (var.isBound()) {
      moveTriggerInt(var, 0, DomainChanged);
      propagateDynInt(0);
    } else {
      for (SysInt i = 0; i < (SysInt)vals.size(); ++i)
        var.removeFromDomain(vals[i]);
    }
  }

  virtual void propagateDynInt(SysInt dt) {
    PROP_INFO_ADDONE(WatchInSet);
    // If we are in here, we have a bounds variable.
    D_ASSERT(var.isBound());
    // lower loop
    SysInt lower_index = 0;

    while (lower_index < (SysInt)vals.size() && vals[lower_index] <= var.getMin()) {
      var.setMin(vals[lower_index] + 1);
      lower_index++;
    }

    SysInt upper_index = (SysInt)vals.size() - 1;

    while (upper_index > 0 && vals[upper_index] >= var.getMax()) {
      var.setMax(vals[upper_index] - 1);
      upper_index--;
    }
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == 1);
    return !binary_search(vals.begin(), vals.end(), v[0]);
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    /// TODO: Make faster
    for (DomainInt i = var.getMin(); i <= var.getMax(); ++i) {
      if (var.inDomain(i) && !binary_search(vals.begin(), vals.end(), i)) {
        assignment.push_back(make_pair(0, i));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint *reverse_constraint() {
    return new WatchInSetConstraint<Var>(var, vals);
  }
};

// From dynamic_inset.h
template <typename Var>
AbstractConstraint *WatchInSetConstraint<Var>::reverse_constraint() {
  return new WatchNotInSetConstraint<Var>(var, vals);
}

template <typename VarArray1>
AbstractConstraint *BuildCT_WATCHED_NOT_INSET(const VarArray1 &_var_array_1,
                                              const ConstraintBlob &b) {
  return new WatchNotInSetConstraint<typename VarArray1::value_type>(_var_array_1[0],
                                                                     b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-notinset",
    "internal_name": "CT_WATCHED_NOT_INSET",
    "args": [ "read_var", "read_constant_list" ]
  }
*/
#endif
