// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0





#ifndef CONSTRAINT_DYNAMIC_UNARY_INRANGE_H
#define CONSTRAINT_DYNAMIC_UNARY_INRANGE_H

// Checks if a variable is in a fixed Range.
template <typename Var>
struct WatchInRangeConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-inrange";
  }

  CONSTRAINT_ARG_LIST2(var, makeVec(rangeMin, rangeMax));
  Var var;

  DomainInt rangeMin;
  DomainInt rangeMax;

  template <typename T>
  WatchInRangeConstraint(const Var& _var, const T& _vals) : var(_var) {
    if(_vals.size() != 2) {
      outputFatalError("The range of an 'inrange' constraint must contain 2 values!");
    }

    rangeMin = _vals[0];
    rangeMax = _vals[1];
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    var.setMin(rangeMin);
    var.setMax(rangeMax);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInRange);
    D_FATAL_ERROR("Propagation is never called for 'in range'");
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] >= rangeMin && v[0] <= rangeMax);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    /// TODO: Make faster
    DomainInt minVal = max(rangeMin, var.min());
    DomainInt maxVal = min(rangeMax, var.max());
    for(DomainInt i = minVal; i <= maxVal; ++i) {
      if(var.inDomain(i)) {
        assignment.push_back(make_pair(0, i));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint();
};

// To get reverseConstraint
#include "dynamic_notinrange.h"

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_INRANGE(const VarArray1& _varArray_1,
                                            const ConstraintBlob& b) {
  return new WatchInRangeConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                    b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-inrange",
    "internal_name": "CT_WATCHED_INRANGE",
    "args": [ "read_var", "read_constant_list" ]
  }
*/
#endif
