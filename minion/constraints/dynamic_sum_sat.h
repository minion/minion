// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_DYNAMIC_SUM_SAT_H
#define CONSTRAINT_DYNAMIC_SUM_SAT_H

template <typename VarArray>
struct BoolSATConstraintDynamic : public AbstractConstraint {
  virtual string constraintName() {
    return "watchsumgeq";
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new BoolLessSumConstraintDynamic<VarArray, DomainInt, 1>(varArray, varArray.size());
  }

  typedef typename VarArray::value_type VarRef;

  CONSTRAINT_ARG_LIST2(varArray, (DomainInt)1);

  VarArray varArray;

  SysInt last;

  BoolSATConstraintDynamic(const VarArray& _varArray) : varArray(_varArray) {
    last = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    SysInt arraySize = varArray.size();
    SysInt trig1, trig2;
    SysInt index = 0;

    while(index < arraySize && !varArray[index].inDomain(1))
      ++index;

    trig1 = index;

    if(index == arraySize) { // Not enough triggers
      getState().setFailed(true);
      return;
    }

    ++index;

    while(index < arraySize && !varArray[index].inDomain(1))
      ++index;

    trig2 = index;

    if(index >= arraySize) { // Only one valid variable.
      varArray[trig1].assign(1);
      return;
    }

    triggerInfo(0) = trig1;
    moveTriggerInt(varArray[trig1], 0, UpperBound);

    triggerInfo(1) = trig2;
    moveTriggerInt(varArray[trig2], 1, UpperBound);

    return;
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(DynSumSat);
    SysInt varSize = varArray.size();

    SysInt other_propval;

    if(0 == dt)
      other_propval = triggerInfo(1);
    else
      other_propval = triggerInfo(0);

    // I thought this would make the code go faster. But it doesn't!
    //  if(varArray[other_propval].isAssignedValue(1))
    //    return;

    bool foundNewSupport = false;

    SysInt loop = last;

    while(loop < varSize && !foundNewSupport) {
      if(loop != other_propval && varArray[loop].inDomain(1))
        foundNewSupport = true;
      else
        ++loop;
    }

    if(!foundNewSupport) {
      loop = 0;

      while(loop < last && !foundNewSupport) {
        if(loop != other_propval && varArray[loop].inDomain(1))
          foundNewSupport = true;
        else
          ++loop;
      }

      if(!foundNewSupport) { // Have to propagate!
        varArray[other_propval].assign(1);
        return;
      }
    }

    // Found new value to watch
    triggerInfo(dt) = loop;
    last = loop;
    moveTriggerInt(varArray[loop], dt, UpperBound);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    SysInt count = 0;
    for(SysInt i = 0; i < vSize; ++i)
      count += (v[i] == 1);
    return count > 0;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(AnyVarRef(varArray[i]));
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(varArray[i].inDomain(1)) {
        assignment.push_back(make_pair(i, 1));
        return true;
      }
    }
    return false;
  }
};

template <typename VarArray>
AbstractConstraint* BoolSATConDynamic(const VarArray& _varArray) {
  return new BoolSATConstraintDynamic<VarArray>(_varArray);
}

#endif
