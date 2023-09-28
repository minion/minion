// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0





#ifndef CONSTRAINT_DYNAMIC_LESS_H
#define CONSTRAINT_DYNAMIC_LESS_H

// var1 < var2
template <typename Var1, typename Var2, bool Negated = false>
struct WatchLessConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "watchless";
  }

  Var1 var1;
  Var2 var2;

  CONSTRAINT_ARG_LIST2(var1, var2);

  WatchLessConstraint(const Var1& _var1, const Var2& _var2) : var1(_var1), var2(_var2) {}

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    moveTriggerInt(var1, 0, LowerBound);
    moveTriggerInt(var2, 1, UpperBound);

    var2.setMin(var1.min() + 1);
    var1.setMax(var2.max() - 1);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchNEQ);

    D_ASSERT(dt == 0 || dt == 1);

    if(dt == 0) {
      var2.setMin(var1.min() + 1);
    } else {
      var1.setMax(var2.max() - 1);
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 2);
    return v[0] < v[1];
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(var1.min() < var2.max()) {
      assignment.push_back(make_pair(0, var1.min()));
      assignment.push_back(make_pair(1, var2.max()));
      return true;
    }
    return false;
  }

  template <bool b, typename T>
  typename std::enable_if<b, AbstractConstraint*>::type
  rev_implement(const ShiftVar<T, compiletimeVal<SysInt, 1>>& var2) {
    return new WatchLessConstraint<T, Var1, false>(var2.data, var1);
  }

  template <bool b, typename T>
  typename std::enable_if<b, AbstractConstraint*>::type rev_implement(const T& var2) {
    return new WatchLessConstraint<AnyVarRef, AnyVarRef, true>(
        var2, ShiftVar<Var1, compiletimeVal<SysInt, 1>>(var1, compiletimeVal<SysInt, 1>()));
  }

  template <bool b, typename T>
  typename std::enable_if<!b, AbstractConstraint*>::type rev_implement(const T& var2) {
    return new WatchLessConstraint<Var2, ShiftVar<Var1, compiletimeVal<SysInt, 1>>, true>(
        var2, ShiftVar<Var1, compiletimeVal<SysInt, 1>>(var1, compiletimeVal<SysInt, 1>()));
  }

  virtual AbstractConstraint* reverseConstraint() {
    return rev_implement<Negated>(var2);
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_WATCHED_LESS(const VarArray1& _varArray_1,
                                         const VarArray2& _varArray_2, ConstraintBlob&) {
  return new WatchLessConstraint<typename VarArray1::value_type, typename VarArray2::value_type>(
      _varArray_1[0], _varArray_2[0]);
}

/* JSON
  { "type": "constraint",
    "name": "watchless",
    "internal_name": "CT_WATCHED_LESS",
    "args": [ "read_var", "read_var" ]
  }
*/
#endif
