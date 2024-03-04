// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_SUM_H_FGHJ
#define CONSTRAINT_SUM_H_FGHJ

// VarToCount = 1 means leq, = 0 means geq.
template <typename VarArray, typename VarSum, SysInt VarToCount = 1>
struct BoolLessSumConstraint : public AbstractConstraint {
  virtual string constraintName() {
    if(VarToCount)
      return "sumleq";
    else
      return "sumgeq";
  }

  CONSTRAINT_ARG_LIST2(varArray, ConstantVar(varSum));

  typedef BoolLessSumConstraint<VarArray, VarSum, 1 - VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  ReversibleInt count;
  VarArray varArray;

  VarSum varSum;

  BoolLessSumConstraint(const VarArray& _varArray, VarSum _varSum)
      : count(), varArray(_varArray), varSum(_varSum) {
    CHECK((VarToCount == 0) || (VarToCount == 1), "Fatal Internal Bug");
    BigInt accumulator = 0;
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      accumulator += checked_cast<SysInt>(
          (DomainInt)max(abs(varArray[i].initialMax()), abs(varArray[i].initialMin())));
      CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");
    }
    accumulator += checked_cast<SysInt>((DomainInt)abs(varSum));
    CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");

    count = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return varArray.size();
  }

  void setupTriggers() {
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      moveTriggerInt(varArray[i], i, VarToCount ? LowerBound : UpperBound);
    }
  }

  virtual AbstractConstraint* reverseConstraint() {
    if(VarToCount)
      return new BoolLessSumConstraint<VarArray, SysInt, 0>(varArray, varSum + 1);
    else
      return new BoolLessSumConstraint<VarArray, SysInt, 1>(varArray, varSum - 1);
  }

  DomainInt occCount() {
    if(VarToCount)
      return varSum;
    else
      return (SysInt)varArray.size() - varSum;
  }

  void limit_reached() {
    SysInt oneVars = 0;
    typename VarArray::value_type* it = &*varArray.begin();
    typename VarArray::value_type* end_it = it + varArray.size();
    for(; it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == VarToCount)
          ++oneVars;
      } else {
        it->uncheckedAssign(1 - VarToCount);
      }
    }
    // D_ASSERT(oneVars >= occCount());
    if(oneVars > occCount())
      getState().setFailed();
  }

  virtual void propagateDynInt(SysInt i, DomainDelta) {
    PROP_INFO_ADDONE(BoolSum);
    D_ASSERT(varArray[checked_cast<SysInt>(i)].assignedValue() == 0 ||
             varArray[checked_cast<SysInt>(i)].assignedValue() == 1);
    SysInt c = count + 1;
    count = c;
    if(c == occCount())
      limit_reached();
  }

  virtual void fullPropagate() {
    setupTriggers();
    SysInt occs = 0;
    SysInt arraySize = varArray.size();
    for(SysInt i = 0; i < arraySize; ++i)
      if(varArray[i].isAssignedValue(VarToCount))
        occs++;
    count = occs;
    if(occs > occCount())
      getState().setFailed();
    if(occs == occCount())
      limit_reached();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    for(SysInt i = 0; i < vSize; i++)
      D_ASSERT(v[i] == 0 || v[i] == 1);
    if(VarToCount)
      return std::accumulate(v, v + vSize, DomainInt(0)) <= varSum;
    else
      return std::accumulate(v, v + vSize, DomainInt(0)) >= varSum;
  }

  /*
  // TODO : Optimise for booleans
  virtual bool getSatisfyingAssignment(box<pair<SysInt,DomainInt> >&
  assignment)
  {
    SysInt sumValue = 0;
    SysInt vSize = varArray.size();
    if(VarToCount)
    {
      for(SysInt i = 0; i < vSize; ++i)
      {
        assignment.push_back(make_pair(i, varArray[i].min()));
        sumValue += varArray[i].min();
      }
      return (sumValue <= varSum);
    }
    else
    {
      for(SysInt i = 0; i < vSize; ++i)
      {
        assignment.push_back(make_pair(i, varArray[i].max()));
        sumValue += varArray[i].max();
      }
      return (sumValue >= varSum);
    }
  }
  */

  // TODO : Optimise for booleans
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    SysInt vSize = varArray.size();
    DomainInt sum_limit;
    if(VarToCount)
      sum_limit = (SysInt)varArray.size() - varSum;
    else
      sum_limit = varSum;

    SysInt ValToFind = 1 - VarToCount;

    SysInt valCount = 0;

    for(SysInt i = 0; i < vSize && valCount < sum_limit; ++i) {
      if(varArray[i].inDomain(ValToFind)) {
        valCount++;
        assignment.push_back(make_pair(i, ValToFind));
      }
    }
    return valCount >= sum_limit;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(varArray[i]);
    return vars;
  }
};

template <typename VarArray, typename VarSum>
AbstractConstraint* BoolLessEqualSumCon(const VarArray& _varArray, VarSum _varSum) {
  return (new BoolLessSumConstraint<VarArray, VarSum>(_varArray, _varSum));
}

template <typename VarArray, typename VarSum>
AbstractConstraint* BoolGreaterEqualSumCon(const VarArray& _varArray, VarSum _varSum) {
  return (new BoolLessSumConstraint<VarArray, VarSum, 0>(_varArray, _varSum));
}

#endif
