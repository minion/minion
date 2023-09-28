// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0





#ifndef CONSTRAINT_DYNAMIC_UNARY_NOTINRANGE_H
#define CONSTRAINT_DYNAMIC_UNARY_NOTINRANGE_H

#include "dynamic_inrange.h"

// Checks if a variable is in a fixed Range.
template <typename Var>
struct WatchNotInRangeConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-notinrange";
  }

  CONSTRAINT_ARG_LIST2(var, makeVec(rangeMin, rangeMax));
  Var var;

  DomainInt rangeMin;
  DomainInt rangeMax;

  template <typename T>
  WatchNotInRangeConstraint(const Var& _var, const T& _vals) : var(_var) {
    if(_vals.size() != 2) {
      cerr << "The range of an 'NotInRange' constraint must contain 2 values!" << endl;
      abort();
    }

    rangeMin = _vals[0];
    rangeMax = _vals[1];
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    // Ignore empty ranges
    if(rangeMin > rangeMax)
      return;

    if(var.max() <= rangeMax) {
      var.setMax(rangeMin - 1);
      return;
    }

    if(var.min() >= rangeMin) {
      var.setMin(rangeMax + 1);
      return;
    }

    if(var.isBound()) {
      moveTriggerInt(var, 0, DomainChanged);
      propagateDynInt(0, DomainDelta::empty());
    } else {
      for(DomainInt i = rangeMin; i <= rangeMax; ++i)
        var.removeFromDomain(i);
    }
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchNotInRange);
    D_ASSERT(var.isBound());

    if(var.max() <= rangeMax) {
      var.setMax(rangeMin - 1);
      return;
    }

    if(var.min() >= rangeMin) {
      var.setMin(rangeMax + 1);
      return;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] < rangeMin || v[0] > rangeMax);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    /// TODO: Make faster
    if(var.min() < rangeMin || var.min() > rangeMax) {
      assignment.push_back(make_pair(0, var.min()));
      return true;
    }

    if(var.max() < rangeMin || var.max() > rangeMax) {
      assignment.push_back(make_pair(0, var.max()));
      return true;
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    std::array<DomainInt, 2> a = {{rangeMin, rangeMax}};
    return new WatchInRangeConstraint<Var>(var, a);
  }
};

// From dynamic_inrange.h
template <typename Var>
AbstractConstraint* WatchInRangeConstraint<Var>::reverseConstraint() {
  std::array<DomainInt, 2> a = {{rangeMin, rangeMax}};
  return new WatchNotInRangeConstraint<Var>(var, a);
}

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_NOT_INRANGE(const VarArray1& _varArray_1,
                                                const ConstraintBlob& b) {
  return new WatchNotInRangeConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                       b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-notinrange",
    "internal_name": "CT_WATCHED_NOT_INRANGE",
    "args": [ "read_var", "read_constant_list" ]
  }
*/

#endif
