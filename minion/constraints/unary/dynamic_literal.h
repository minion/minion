// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0





#ifndef CONSTRAINT_DYNAMIC_UNARY_LITERAL_H
#define CONSTRAINT_DYNAMIC_UNARY_LITERAL_H

#include "dynamic_notliteral.h"

// Checks if a variable is equal to a value.
template <typename Var>
struct WatchLiteralConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-literal";
  }

  CONSTRAINT_ARG_LIST2(var, val);
  Var var;

  DomainInt val;

  template <typename T>
  WatchLiteralConstraint(const Var& _var, const T& _val) : var(_var), val(_val) {}

  virtual SysInt dynamicTriggerCount() {
    return 0;
  }

  virtual void fullPropagate() {
    var.assign(val);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInRange);
    var.assign(val);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] == val);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(var.inDomain(val)) {
      assignment.push_back(make_pair(0, val));
      return true;
    } else
      return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new WatchNotLiteralConstraint<Var>(var, val);
  }
};

// From dynamic_notliteral.h
template <typename Var>
AbstractConstraint* WatchNotLiteralConstraint<Var>::reverseConstraint() {
  return new WatchLiteralConstraint<Var>(var, val);
}

inline AbstractConstraint* WatchNotLiteralBoolConstraint::reverseConstraint() {
  return new WatchLiteralConstraint<BoolVarRef>(var, val);
}

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_LIT(const VarArray1& _varArray_1, const ConstraintBlob& b) {
  return new WatchLiteralConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                    b.constants[0][0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-literal",
    "internal_name": "CT_WATCHED_LIT",
    "args": [ "read_var", "read_constant" ]
  }
*/

#endif
