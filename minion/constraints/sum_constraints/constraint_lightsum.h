/*
 * Minion http://minion.sourceforge.net
 * Copyright (C) 2006-09
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

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

  CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2("weighted", "sumleq", "sumgeq", varArray, var_sum);

  bool no_negatives;

  std::array<VarRef, size> varArray;
  VarSum var_sum;
  LightLessEqualSumConstraint(const std::array<VarRef, size>& _varArray, const VarSum& _var_sum)
      : varArray(_varArray), var_sum(_var_sum) {
    BigInt accumulator = 0;
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      accumulator += checked_cast<SysInt>(
          max(abs(varArray[i].initialMax()), abs(varArray[i].initialMin())));
      CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");
    }
    accumulator +=
        checked_cast<SysInt>(max(abs(var_sum.initialMax()), abs(var_sum.initialMin())));
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

  void setup_triggers() {
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      moveTriggerInt(varArray[i], i, LowerBound);
    }
    moveTriggerInt(var_sum, varArray.size(), UpperBound);
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt sumValue = 0;
    SysInt vSize = varArray.size();

    if(no_negatives) {
      DomainInt max_sum = var_sum.max();
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
      if(sumValue > var_sum.max())
        return false;
      else
        assignment.push_back(make_pair(vSize, var_sum.max()));
      return true;
    }
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(LightSum);
    DomainInt min_sum = 0;
    for(UnsignedSysInt i = 0; i < size; ++i)
      min_sum += varArray[i].min();

    if(propVal != varArray.size()) {
      var_sum.setMin(min_sum);
    }

    DomainInt slack = var_sum.max() - min_sum;
    for(UnsignedSysInt i = 0; i < size; ++i)
      varArray[i].setMax(varArray[i].min() + slack);
  }

  virtual void fullPropagate() {
    setup_triggers();
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
    vector<AnyVarRef> array_copy;
    array_copy.reserve(varArray.size() + 1);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      array_copy.push_back(varArray[i]);
    array_copy.push_back(var_sum);
    return array_copy;
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
    SumType new_sum = ShiftVarRef(VarNegRef(var_sum), compiletimeVal<SysInt, -1>());

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
    SumType new_sum = ShiftVarRef(VarNegRef(var_sum), compiletimeVal<SysInt, -1>());

    return new LightLessEqualSumConstraint<AnyVarRef, size, AnyVarRef, true>(new_varArray,
                                                                             new_sum);
  }
};

template <typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint* LightLessEqualSumCon(const std::array<VarRef, size>& _varArray,
                                         const VarSum& _var_sum) {
  return (new LightLessEqualSumConstraint<VarRef, size, VarSum>(_varArray, _var_sum));
}

template <typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint* LightGreaterEqualSumCon(const std::array<VarRef, size>& _varArray,
                                            const VarSum& _var_sum) {
  return (new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size,
                                          typename NegType<VarSum>::type>(VarNegRef(_varArray),
                                                                          VarNegRef(_var_sum)));
}

#endif
