// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0









#ifndef CONSTRAINT_NEQ_H
#define CONSTRAINT_NEQ_H

#include "constraint_checkassign.h"

template <typename VarArray>
struct NeqConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "alldiff";
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  VarArray varArray;

  CONSTRAINT_ARG_LIST1(varArray);

  NeqConstraint(const VarArray& _varArray) : varArray(_varArray) {}

  virtual SysInt dynamicTriggerCount() {
    return varArray.size();
  }

  void setupTriggers() {
    for(SysInt i = 0; i < varArray.size(); ++i)
      moveTriggerInt(varArray[i], i, Assigned);
  }

  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }

  virtual void propagateDynInt(SysInt propVal_in, DomainDelta) {
    const SysInt propVal = checked_cast<SysInt>(propVal_in);
    PROP_INFO_ADDONE(ArrayNeq);
    DomainInt removeVal = varArray[propVal].assignedValue();
    SysInt arraySize = varArray.size();
    for(SysInt i = 0; i < arraySize; ++i) {
      if(i != propVal) {
        if(varArray[i].isBound()) {
          if(varArray[i].min() == removeVal)
            varArray[i].setMin(removeVal + 1);
          if(varArray[i].max() == removeVal)
            varArray[i].setMax(removeVal - 1);
        } else {
          varArray[i].removeFromDomain(removeVal);
        }
      }
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    SysInt arraySize = varArray.size();
    for(SysInt i = 0; i < arraySize; ++i)
      if(varArray[i].isAssigned()) {
        DomainInt removeVal = varArray[i].assignedValue();
        for(SysInt j = 0; j < arraySize; ++j) {
          if(i != j) {
            if(varArray[j].isBound()) {
              if(varArray[j].min() == removeVal)
                varArray[j].setMin(removeVal + 1);
              if(varArray[j].max() == removeVal)
                varArray[j].setMax(removeVal - 1);
            } else {
              varArray[j].removeFromDomain(removeVal);
            }
          }
        }
      }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    SysInt arraySize = checked_cast<SysInt>(vSize);
    for(SysInt i = 0; i < arraySize; i++)
      for(SysInt j = i + 1; j < arraySize; j++)
        if(v[i] == v[j])
          return false;
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(varArray[i]);
    return vars;
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, varArray.size());

    for(UnsignedSysInt i = 0; i < varArray.size(); ++i) {
      if(!varArray[i].isAssigned()) {
        assignment.push_back(make_pair(i, varArray[i].min()));
        assignment.push_back(make_pair(i, varArray[i].max()));
        return true;
      } else
        c.push_back(varArray[i].assignedValue());
    }

    if(checkAssignment(c.begin(), c.size())) { // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)varArray.size(); ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }
};

template <typename VarArray>
AbstractConstraint* BuildCT_ALLDIFF(const VarArray& varArray, ConstraintBlob&) {
  return new NeqConstraint<VarArray>(varArray);
}

/* JSON
{ "type": "constraint",
  "name": "alldiff",
  "internal_name": "CT_ALLDIFF",
  "args": [ "read_list" ]
}
*/

#endif
