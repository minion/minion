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

/** @help constraints;sumleq Description
The constraint

   sumleq(vec, c)

ensures that sum(vec) <= c.
*/

/** @help constraints;sumgeq Description
The constraint

   sumgeq(vec, c)

ensures that sum(vec) >= c.
*/

// This is the standard implementation of sumleq (and sumgeq)

#ifndef CONSTRAINT_FULLSUM_H
#define CONSTRAINT_FULLSUM_H

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

/// V1 + ... Vn <= X
/// is_reversed checks if we are in the case where reverseConstraint was
/// previously called.
template <typename VarArray, typename VarSum, BOOL is_reversed = false>
struct LessEqualSumConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "sumleq";
  }

  CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2("weighted", "sumleq", "sumgeq", varArray, varSum);

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  bool no_negatives;

  VarArray varArray;
  VarSum varSum;
  DomainInt max_looseness;
  Reversible<DomainInt> varArrayMinSum;
  LessEqualSumConstraint(const VarArray& _varArray, VarSum _varSum)
      : varArray(_varArray), varSum(_varSum), varArrayMinSum() {
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

  DomainInt getRealMinSum() {
    DomainInt min_sum = 0;
    for(typename VarArray::iterator it = varArray.begin(); it != varArray.end(); ++it)
      min_sum += it->min();
    return min_sum;
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta domainChange) {
    P("Prop: " << propVal);
    PROP_INFO_ADDONE(FullSum);
    DomainInt sum = varArrayMinSum;
    if(propVal != varArray.size()) { // One of the array changed
      DomainInt change = varArray[checked_cast<SysInt>(propVal)].getDomainChange(domainChange);
      P(" Change: " << change);
      D_ASSERT(change >= 0);
      sum += change;
      varArrayMinSum = sum;
    }

    varSum.setMin(sum);
    if(getState().isFailed())
      return;
    D_ASSERT(sum <= getRealMinSum());

    DomainInt looseness = varSum.max() - sum;
    if(looseness < 0) {
      getState().setFailed(true);
      return;
    }

    if(looseness < max_looseness) {
      for(typename VarArray::iterator it = varArray.begin(); it != varArray.end(); ++it)
        it->setMax(it->min() + looseness);
    }
  }

  virtual void fullPropagate() {
    P("Full Prop");
    setupTriggers();
    DomainInt min_sum = 0;
    DomainInt maxDiff = 0;
    for(typename VarArray::iterator it = varArray.begin(); it != varArray.end(); ++it) {
      min_sum += it->min();
      maxDiff = max(maxDiff, it->max() - it->min());
    }

    varArrayMinSum = min_sum;
    D_ASSERT(min_sum == getRealMinSum());
    max_looseness = maxDiff;
    if(!varArray.empty())
      propagateDynInt(0, DomainDelta::empty());
    else
      varSum.setMin(0);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size() + 1);
    DomainInt sum = 0;
    for(SysInt i = 0; i < vSize - 1; i++)
      sum += v[i];
    return sum <= *(v + vSize - 1);
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    P("GSA");
    DomainInt sumValue = 0;
    SysInt vSize = varArray.size();

    if(no_negatives) // How are the two cases different? They look identical.
    {
      DomainInt max_sum = varSum.max();
      assignment.push_back(make_pair(vSize, max_sum));
      for(SysInt i = 0; i < vSize && sumValue <= max_sum; ++i) {
        DomainInt minVal = varArray[i].min();
        assignment.push_back(make_pair(i, minVal));
        sumValue += minVal;
      }
      P("A" << (sumValue <= max_sum));
      return (sumValue <= max_sum);
    } else {
      for(SysInt i = 0; i < vSize; ++i) {
        assignment.push_back(make_pair(i, varArray[i].min()));
        sumValue += varArray[i].min();
      }
      P("B" << (sumValue <= varSum.max()));
      if(sumValue > varSum.max())
        return false;
      else
        assignment.push_back(make_pair(vSize, varSum.max()));
      return true;
    }
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> arrayCopy(varArray.size() + 1);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      arrayCopy[i] = varArray[i];
    arrayCopy[varArray.size()] = varSum;
    return arrayCopy;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return rev_implement<is_reversed>();
  }

  template <bool b>
  typename std::enable_if<!b, AbstractConstraint*>::type rev_implement() {
    typename NegType<VarArray>::type new_varArray(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      new_varArray[i] = VarNegRef(varArray[i]);

    typedef typename ShiftType<typename NegType<VarSum>::type, compiletimeVal<SysInt, -1>>::type
        SumType;
    SumType new_sum = ShiftVarRef(VarNegRef(varSum), compiletimeVal<SysInt, -1>());

    return new LessEqualSumConstraint<typename NegType<VarArray>::type, SumType, true>(
        new_varArray, new_sum);
  }

  template <bool b>
  typename std::enable_if<b, AbstractConstraint*>::type rev_implement() {
    vector<AnyVarRef> new_varArray(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      new_varArray[i] = VarNegRef(varArray[i]);

    typedef typename ShiftType<typename NegType<VarSum>::type, compiletimeVal<SysInt, -1>>::type
        SumType;
    SumType new_sum = ShiftVarRef(VarNegRef(varSum), compiletimeVal<SysInt, -1>());

    return new LessEqualSumConstraint<vector<AnyVarRef>, AnyVarRef, true>(new_varArray, new_sum);
  }
};

#endif
