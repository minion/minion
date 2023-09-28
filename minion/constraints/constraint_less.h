// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0





#ifndef CONSTRAINT_LESS_H
#define CONSTRAINT_LESS_H

// x <= y + offset
template <typename VarRef1, typename VarRef2, typename Offset>
struct LeqConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "ineq";
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  VarRef1 x;
  VarRef2 y;
  const Offset offset;

  CONSTRAINT_ARG_LIST3(x, y, offset);

  LeqConstraint(VarRef1 _x, VarRef2 _y, Offset _o) : x(_x), y(_y), offset(_o) {}

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  // Needs to be at end of file
  virtual AbstractConstraint* reverseConstraint();

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(BinaryLeq);
    if(checked_cast<SysInt>(propVal)) { // y changed
      x.setMax(y.max() + offset);
    } else { // x changed
      y.setMin(x.min() - offset);
    }
  }

  virtual void fullPropagate() {
    moveTriggerInt(x, 0, LowerBound);
    moveTriggerInt(y, 1, UpperBound);

    propagateDynInt(0, DomainDelta::empty());
    propagateDynInt(1, DomainDelta::empty());
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 2);
    return v[0] <= (v[1] + offset);
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt xMin = x.min();
    DomainInt yMax = y.max();

    if(xMin <= yMax + offset) {
      assignment.push_back(make_pair(0, xMin));
      assignment.push_back(make_pair(1, yMax));
      return true;
    }
    return false;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> array;
    array.reserve(2);
    array.push_back(x);
    array.push_back(y);
    return array;
  }
};

template <typename VarRef1, typename VarRef2, typename Offset>
AbstractConstraint* LeqCon(VarRef1 v1, VarRef2 v2, Offset o) {
  return new LeqConstraint<VarRef1, VarRef2, Offset>(v1, v2, o);
}

template <typename VarRef1, typename VarRef2>
AbstractConstraint* LeqCon(VarRef1 v1, VarRef2 v2) {
  return new LeqConstraint<VarRef1, VarRef2, compiletimeVal<SysInt, 0>>(
      v1, v2, compiletimeVal<SysInt, 0>());
}

template <typename VarRef>
AbstractConstraint* ImpliesCon(VarRef v1, VarRef v2) {
  return new LeqConstraint<VarRef, VarRef, compiletimeVal<SysInt, 0>>(
      v1, v2, compiletimeVal<SysInt, 0>());
}

// This is mainly inline to avoid multiple definitions.
template <typename VarRef1, typename VarRef2, typename Offset>
inline AbstractConstraint* LeqConstraint<VarRef1, VarRef2, Offset>::reverseConstraint() {
  return LeqCon(y, x, const_negminusone(offset));
}

template <typename T1, typename T2>
AbstractConstraint* BuildCT_INEQ(const T1& t1, const T2& t2, ConstraintBlob& b) {
  return LeqCon(t1[0], t2[0], b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "ineq",
  "internal_name": "CT_INEQ",
  "args": [ "read_var", "read_var", "read_constant" ]
}
*/

#endif
