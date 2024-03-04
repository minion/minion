// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_CONSTANT_H
#define CONSTRAINT_CONSTANT_H

template <bool truth>
struct ConstantConstraint : public AbstractConstraint {

  virtual string constraintName() {
    if(truth)
      return "true";
    else
      return "false";
  }

  CONSTRAINT_ARG_LIST0();

  ConstantConstraint() {}

  virtual void fullPropagate() {
    if(!truth)
      getState().setFailed();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 0);
    return truth;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    return truth;
  }

  AbstractConstraint* reverseConstraint() {
    return new ConstantConstraint<!truth>();
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> v;
    return v;
  }
};

inline AbstractConstraint* BuildCT_TRUE(ConstraintBlob&) {
  return (new ConstantConstraint<true>());
}

inline AbstractConstraint* BuildCT_FALSE(ConstraintBlob&) {
  return (new ConstantConstraint<false>());
}

/* JSON
{ "type": "constraint",
  "name": "false",
  "internal_name": "CT_FALSE",
  "args": [ ]
}
*/

/* JSON
{ "type": "constraint",
  "name": "true",
  "internal_name": "CT_TRUE",
  "args": [ ]
}
*/

#endif
