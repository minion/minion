// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_DYNAMIC_NEQ_H
#define CONSTRAINT_DYNAMIC_NEQ_H

#include "constraint_equal.h"

template <typename Var1, typename Var2>
struct WatchNeqConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "watchneq";
  }

  Var1 var1;
  Var2 var2;

  CONSTRAINT_ARG_LIST2(var1, var2);

  WatchNeqConstraint(const Var1& _var1, const Var2& _var2) : var1(_var1), var2(_var2) {
    CheckNotBoundSingle(var1, "watchneq", "neq");
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    if(var1.isAssigned() && var2.isAssigned() &&
       var1.assignedValue() == var2.assignedValue()) {
      getState().setFailed();
      return;
    }

    if(var1.isAssigned()) {
      var2.removeFromDomain(var1.assignedValue());
      return;
    }

    if(var2.isAssigned()) {
      var1.removeFromDomain(var2.assignedValue());
      return;
    }

    moveTriggerInt(var1, 0, Assigned);
    moveTriggerInt(var2, 1, Assigned);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchNEQ);

    D_ASSERT(dt == 0 || dt == 1);

    if(dt == 0) {
      D_ASSERT(var1.isAssigned());
      var2.removeFromDomain(var1.assignedValue());
    } else {
      D_ASSERT(var2.isAssigned());
      var1.removeFromDomain(var2.assignedValue());
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 2);
    return v[0] != v[1];
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(var1.isAssigned() && var2.isAssigned() && var1.assignedValue() == var2.assignedValue())
      return false;

    if(var1.isAssigned()) {
      assignment.push_back(make_pair(0, var1.assignedValue()));
      if(var2.min() != var1.assignedValue())
        assignment.push_back(make_pair(1, var2.min()));
      else
        assignment.push_back(make_pair(1, var2.max()));
    } else {
      assignment.push_back(make_pair(1, var2.min()));
      if(var1.min() != var2.min())
        assignment.push_back(make_pair(0, var1.min()));
      else
        assignment.push_back(make_pair(0, var1.max()));
    }
    return true;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new EqualConstraint<Var1, Var2>(var1, var2);
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_WATCHED_NEQ(const VarArray1& _varArray_1,
                                        const VarArray2& _varArray_2, ConstraintBlob&) {
  return new WatchNeqConstraint<typename VarArray1::value_type, typename VarArray2::value_type>(
      _varArray_1[0], _varArray_2[0]);
}

/* JSON
  { "type": "constraint",
    "name": "watchneq",
    "internal_name": "CT_WATCHED_NEQ",
    "args": [ "read_var", "read_var" ]
  }
*/
#endif
