// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0





#ifndef CONSTRAINT_DYNAMIC_UNARY_NOT_INLITERAL_H
#define CONSTRAINT_DYNAMIC_UNARY_NOT_INLITERAL_H

// Checks if a variable is equal to a value.
template <typename Var>
struct WatchNotLiteralConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-notliteral";
  }

  Var var;

  DomainInt val;

  CONSTRAINT_ARG_LIST2(var, val);

  template <typename T>
  WatchNotLiteralConstraint(const Var& _var, const T& _val) : var(_var), val(_val) {}

  virtual SysInt dynamicTriggerCount() {
    return 1;
  }

  virtual void fullPropagate() {
    if(var.isBound()) {
      if(var.min() == val)
        var.setMin(val + 1);
      else if(var.max() == val)
        var.setMax(val - 1);
      else
        moveTriggerInt(var, 0, DomainChanged);
    } else
      var.removeFromDomain(val);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInRange);
    if(var.isBound()) {
      if(var.min() == val)
        var.setMin(val + 1);
      else if(var.max() == val)
        var.setMax(val - 1);
    } else
      var.removeFromDomain(val);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] != val);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    D_ASSERT(var.inDomain(var.min()) && var.inDomain(var.max()));
    DomainInt tmp;
    if((tmp = var.min()) != val) {
      assignment.push_back(make_pair(0, tmp));
      return true;
    } else if((tmp = var.max()) != val) {
      assignment.push_back(make_pair(0, tmp));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverseConstraint();
};

struct WatchNotLiteralBoolConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-notliteral";
  }

  CONSTRAINT_ARG_LIST2(var, val);

  BoolVarRef var;

  DomainInt val;

  template <typename T>
  WatchNotLiteralBoolConstraint(const BoolVarRef& _var, const T& _val) : var(_var), val(_val) {
    // cout << "using boolean specialisation" << endl;
  }

  virtual SysInt dynamicTriggerCount() {
    return 0;
  }

  virtual void fullPropagate() {
    var.removeFromDomain(val);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInRange);
    var.removeFromDomain(val);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] != val);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(var.min() != val) {
      assignment.push_back(make_pair(0, var.min()));
      return true;
    }
    if(var.max() != val) {
      assignment.push_back(make_pair(0, var.max()));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverseConstraint();
};

// For reverse constraint.
#include "dynamic_literal.h"

inline AbstractConstraint* BuildCT_WATCHED_NOTLIT(const vector<BoolVarRef>& vec,
                                                  const ConstraintBlob& b) {
  return new WatchNotLiteralBoolConstraint(vec[0], b.constants[0][0]);
}

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_NOTLIT(const VarArray1& _varArray_1, const ConstraintBlob& b) {
  return new WatchNotLiteralConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                       b.constants[0][0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-notliteral",
    "internal_name": "CT_WATCHED_NOTLIT",
    "args": [ "read_var", "read_constant" ]
  }
*/
#endif
