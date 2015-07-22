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

#ifndef CONSTRAINT_DYNAMIC_NEQ_H
#define CONSTRAINT_DYNAMIC_NEQ_H

#include "constraint_equal.h"

template <typename Var1, typename Var2>
struct WatchNeqConstraint : public AbstractConstraint {
  virtual string constraint_name() { return "watchneq"; }

  Var1 var1;
  Var2 var2;

  CONSTRAINT_ARG_LIST2(var1, var2);

  WatchNeqConstraint(const Var1 &_var1, const Var2 &_var2) : var1(_var1), var2(_var2) {
    CheckNotBoundSingle(var1, "watchneq", "neq");
  }

  virtual SysInt dynamic_trigger_count() { return 2; }

  virtual void full_propagate() {
    if (var1.isAssigned() && var2.isAssigned() &&
        var1.getAssignedValue() == var2.getAssignedValue()) {
      getState().setFailed(true);
      return;
    }

    if (var1.isAssigned()) {
      var2.removeFromDomain(var1.getAssignedValue());
      return;
    }

    if (var2.isAssigned()) {
      var1.removeFromDomain(var2.getAssignedValue());
      return;
    }

    moveTriggerInt(var1, 0, Assigned);
    moveTriggerInt(var2, 1, Assigned);
  }

  virtual void propagateDynInt(SysInt dt) {
    PROP_INFO_ADDONE(WatchNEQ);

    D_ASSERT(dt == 0 || dt == 1);

    if (dt == 0) {
      D_ASSERT(var1.isAssigned());
      var2.removeFromDomain(var1.getAssignedValue());
    } else {
      D_ASSERT(var2.isAssigned());
      var1.removeFromDomain(var2.getAssignedValue());
    }
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == 2);
    return v[0] != v[1];
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    if (var1.isAssigned() && var2.isAssigned() &&
        var1.getAssignedValue() == var2.getAssignedValue())
      return false;

    if (var1.isAssigned()) {
      assignment.push_back(make_pair(0, var1.getAssignedValue()));
      if (var2.getMin() != var1.getAssignedValue())
        assignment.push_back(make_pair(1, var2.getMin()));
      else
        assignment.push_back(make_pair(1, var2.getMax()));
    } else {
      assignment.push_back(make_pair(1, var2.getMin()));
      if (var1.getMin() != var2.getMin())
        assignment.push_back(make_pair(0, var1.getMin()));
      else
        assignment.push_back(make_pair(0, var1.getMax()));
    }
    return true;
  }

  virtual AbstractConstraint *reverse_constraint() {
    return new EqualConstraint<Var1, Var2>(var1, var2);
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint *BuildCT_WATCHED_NEQ(const VarArray1 &_var_array_1,
                                        const VarArray2 &_var_array_2, ConstraintBlob &) {
  return new WatchNeqConstraint<typename VarArray1::value_type, typename VarArray2::value_type>(
      _var_array_1[0], _var_array_2[0]);
}

/* JSON
  { "type": "constraint",
    "name": "watchneq",
    "internal_name": "CT_WATCHED_NEQ",
    "args": [ "read_var", "read_var" ]
  }
*/
#endif
