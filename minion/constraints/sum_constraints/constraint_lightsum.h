// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_LIGHTSUM_H_PLOKIJ
#define CONSTRAINT_LIGHTSUM_H_PLOKIJ

// This implementation of sum is designed for very short arrays, by
// not keeping any state.

/// V1 + ... Vn <= X
template <typename VarRef, std::size_t size, typename VarSum, BOOL is_reversed = false>
struct LightLessEqualSumConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "Light<=Sum";
  }

  CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2("weighted", "sumleq", "sumgeq", varArray, varSum);

  bool no_negatives;

  std::array<VarRef, size> varArray;
  VarSum varSum;
  LightLessEqualSumConstraint(const std::array<VarRef, size>& _varArray, const VarSum& _varSum)
      : varArray(_varArray), varSum(_varSum) {
    BigInt accumulator = 0;
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      accumulator += checked_cast<SysInt>(
          max(abs(varArray[i].initialMax()), abs(varArray[i].initialMin())));
      CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");
    }
    accumulator +=
        checked_cast<SysInt>(max(abs(varSum.initialMax()), abs(varSum.initialMin())));
    CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");

    no_negatives = true;
    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(varArray[i].initialMin() < 0) {
        no_negatives = false;
        return;
      }
    }
  }

  virtual SysInt dynamicTriggerCount() {
    return varArray.size() + 1;
  }

  void setupTriggers() {
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      moveTriggerInt(varArray[i], i, LowerBound);
    }
    moveTriggerInt(varSum, varArray.size(), UpperBound);
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt sumValue = 0;
    SysInt vSize = varArray.size();

    if(no_negatives) {
      DomainInt max_sum = varSum.max();
      assignment.push_back(make_pair(vSize, max_sum));
      for(SysInt i = 0; i < vSize && sumValue <= max_sum; ++i) {
        DomainInt minVal = varArray[i].min();
        assignment.push_back(make_pair(i, minVal));
        sumValue += minVal;
      }
      return (sumValue <= max_sum);
    } else {
      for(SysInt i = 0; i < vSize; ++i) {
        assignment.push_back(make_pair(i, varArray[i].min()));
        sumValue += varArray[i].min();
      }
      if(sumValue > varSum.max())
        return false;
      else
        assignment.push_back(make_pair(vSize, varSum.max()));
      return true;
    }
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(LightSum);
    DomainInt min_sum = 0;
    for(UnsignedSysInt i = 0; i < size; ++i)
      min_sum += varArray[i].min();

    if(propVal != varArray.size()) {
      varSum.setMin(min_sum);
    }

    DomainInt slack = varSum.max() - min_sum;
    for(UnsignedSysInt i = 0; i < size; ++i)
      varArray[i].setMax(varArray[i].min() + slack);
  }

  virtual void fullPropagate() {
    setupTriggers();
    propagateDynInt(varArray.size(), DomainDelta::empty());
    propagateDynInt(0, DomainDelta::empty());
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size() + 1);
    DomainInt sum = 0;
    for(SysInt i = 0; i < vSize - 1; i++)
      sum += v[i];
    return sum <= *(v + vSize - 1);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> arrayCopy;
    arrayCopy.reserve(varArray.size() + 1);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      arrayCopy.push_back(varArray[i]);
    arrayCopy.push_back(varSum);
    return arrayCopy;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return rev_implement<is_reversed>();
  }

  template <bool b>
  typename std::enable_if<!b, AbstractConstraint*>::type rev_implement() {
    typedef std::array<typename NegType<VarRef>::type, size> VarArray;
    VarArray new_varArray;
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      new_varArray[i] = VarNegRef(varArray[i]);

    typedef typename ShiftType<typename NegType<VarSum>::type, compiletimeVal<SysInt, -1>>::type
        SumType;
    SumType new_sum = ShiftVarRef(VarNegRef(varSum), compiletimeVal<SysInt, -1>());

    return new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size, SumType, true>(
        new_varArray, new_sum);
  }

  template <bool b>
  typename std::enable_if<b, AbstractConstraint*>::type rev_implement() {
    typedef std::array<AnyVarRef, size> VarArray;
    VarArray new_varArray;
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      new_varArray[i] = VarNegRef(varArray[i]);

    typedef typename ShiftType<typename NegType<VarSum>::type, compiletimeVal<SysInt, -1>>::type
        SumType;
    SumType new_sum = ShiftVarRef(VarNegRef(varSum), compiletimeVal<SysInt, -1>());

    return new LightLessEqualSumConstraint<AnyVarRef, size, AnyVarRef, true>(new_varArray,
                                                                             new_sum);
  }
};

template <typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint* LightLessEqualSumCon(const std::array<VarRef, size>& _varArray,
                                         const VarSum& _varSum) {
  return (new LightLessEqualSumConstraint<VarRef, size, VarSum>(_varArray, _varSum));
}

template <typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint* LightGreaterEqualSumCon(const std::array<VarRef, size>& _varArray,
                                            const VarSum& _varSum) {
  return (new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size,
                                          typename NegType<VarSum>::type>(VarNegRef(_varArray),
                                                                          VarNegRef(_varSum)));
}

#endif
