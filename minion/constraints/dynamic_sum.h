// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

/*
  Chris, note there is a bit where I comment out how I really want to propagate
  instead
  of calling limit_reached, but I couldn't remember if you told me I could have
  that method or not.    It saves going through the whole clause.
 */















#ifndef CONSTRAINT_DYNAMIC_SUM_H
#define CONSTRAINT_DYNAMIC_SUM_H

// VarToCount = 1 means leq, = 0 means geq.
template <typename VarArray, typename VarSum, SysInt VarToCount = 1, BOOL is_reversed = false>
struct BoolLessSumConstraintDynamic : public AbstractConstraint {
  virtual string constraintName() {
    if(VarToCount)
      return "watchsumleq";
    else
      return "watchsumgeq";
  }

  typedef BoolLessSumConstraintDynamic<VarArray, VarSum, 1 - VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  CONSTRAINT_ARG_LIST2(varArray,
                       VarToCount ? (DomainInt)(varArray.size()) - varSum : (DomainInt)(varSum));

  // When VarToCount=1 this constraint actually counts 0's and ensures there are
  // varSum or more.
  // Name of the class should really be changed, and VarToCount changed to val..
  // and values flipped
  // for it to make sense.

  VarArray varArray;
  VarSum varSum;

  void* unwatchedIndexes;
  SysInt last;
  SysInt numUnwatched;

  SysInt& unwatched(SysInt i) {
    return static_cast<SysInt*>(unwatchedIndexes)[i];
  }

  BoolLessSumConstraintDynamic(const VarArray& _varArray, VarSum _varSum)
      : varArray(_varArray), varSum(_varSum), unwatchedIndexes(0), last(0) {
    D_ASSERT((VarToCount == 0) || (VarToCount == 1));
    // Sum of 1's is >= K
    // == Number of 1's is >=K         // this is the one I want to do
    // == Number of 0's is <= N-K

    SysInt arraySize = varArray.size();

    if(varSum >= arraySize || varSum < 0) {
      // In these cases the constraints are all set before search.
      // This will happen before triggers set up in fullPropagate
      // Thus zero triggers are needed
    } else {
      numUnwatched = checked_cast<SysInt>(arraySize - varSum - 1);
      D_ASSERT(numUnwatched >= 0);
      unwatchedIndexes = checked_malloc(sizeof(UnsignedSysInt) * numUnwatched);
    }
  }

  ~BoolLessSumConstraintDynamic() {
    free(unwatchedIndexes);
  }

  virtual SysInt dynamicTriggerCount() {
    if(varSum >= (SysInt)varArray.size() || varSum < 0)
      return 0;
    else
      return checked_cast<SysInt>(varSum + 1);
  }

  virtual void fullPropagate() {
    SysInt dt = 0;

    if(varSum <= 0)
      // Constraint trivially satisfied
      return;

    SysInt arraySize = varArray.size();
    DomainInt triggers_wanted = varSum + 1;
    SysInt index;

    for(index = 0; (index < arraySize) && (triggers_wanted > 0); ++index) {
      if(varArray[index].inDomain(1 - VarToCount)) {
        // delay setting up triggers in case we don't need to
        --triggers_wanted;
      }
    }

    D_ASSERT(triggers_wanted >= 0);

    if(triggers_wanted > 1) // Then we have failed, forget it.
    {
      getState().setFailed(true);
      return;
    } else if(triggers_wanted == 1) // Then we can propagate
    {                               // We never even set up triggers
      for(SysInt i = 0; i < arraySize; ++i) {
        if(varArray[i].inDomain(1 - VarToCount)) {
          if(VarToCount)
            varArray[i].setMax(0);
          else
            varArray[i].setMin(1);
        }
      }
    } else // Now set up triggers
    {
      D_ASSERT(triggers_wanted == 0);

      SysInt j = 0;

      // We only look at the elements of vararray that we looked at before
      // Exactly triggers_wanted of them have the val in their domain.

      for(SysInt i = 0; (i < index); ++i) // remember index was the elts we looked at
      {
        if(varArray[i].inDomain(1 - VarToCount)) {
          triggerInfo(dt) = i;
          moveTriggerInt(varArray[i], dt, VarToCount ? LowerBound : UpperBound);
          ++dt;
        } else {
          unwatched(j) = i;
          ++j;
          // When run at root we could optimise as follows
          //    If VarToCount not in domain then do not put j in unwatched
          //      Instead decrement numUnwatched
        }
      }

      for(SysInt i = index; i < arraySize; ++i) {
        unwatched(j) = i;
        ++j;
      }

      D_ASSERT(j == numUnwatched);
    }
    return;
  }

  /// Checks the consistency of the constraint's data structures
  //
  //  NOT updated for new type of watched data structure
  //
  BOOL check_consistency() {
    return true;
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(DynSum);
    D_ASSERT(check_consistency());
    SysInt propval = triggerInfo(dt);
    D_ASSERT(varArray[propval].assignedValue() == VarToCount);
    // should generalise
    // and will need to loop round for watched lits

    bool foundNewSupport = false;

    SysInt loop;
    SysInt j;

    for(loop = 0; (!foundNewSupport) && loop < numUnwatched; ++loop) {
      D_ASSERT(numUnwatched > 0);

      j = (last + 1 + loop) % numUnwatched;
      if(varArray[unwatched(j)].inDomain(1 - VarToCount)) {
        foundNewSupport = true;
      }
    }

    if(foundNewSupport) // so we have found a new literal to watch
    {
      SysInt& unwatchedIndex = unwatched(j);

      triggerInfo(dt) = unwatchedIndex;
      moveTriggerInt(varArray[unwatchedIndex], dt, VarToCount ? LowerBound : UpperBound);

      unwatchedIndex = propval;
      last = j;

      return;
    }

    // there is no literal to watch, we need to propagate

    SysInt dt2 = 0;

    for(SysInt z = 0; z < varSum + 1; ++z) {
      if(dt != dt2) // that one has just been set the other way
      {
        if(VarToCount)
          varArray[triggerInfo(dt2)].setMax(0);
        else
          varArray[triggerInfo(dt2)].setMin(1);
      }
      dt2++;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    SysInt count = 0;
    for(SysInt i = 0; i < vSize; ++i)
      count += (v[i] != VarToCount);
    return count >= varSum;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(AnyVarRef(varArray[i]));
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(varSum <= 0) {
      return true;
    }

    SysInt count = 0;
    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(varArray[i].inDomain(!VarToCount)) {
        assignment.push_back(make_pair(i, !VarToCount));
        count++;
        if(count >= varSum)
          return true;
      }
    }
    // We didn't make a complete assignment
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new BoolLessSumConstraintDynamic<VarArray, VarSum, 1 - VarToCount, true>(
        varArray, varArray.size() - varSum + 1);
  }
};

template <typename VarArray, typename VarSum>
AbstractConstraint* BoolLessEqualSumConDynamic(const VarArray& _varArray, VarSum _varSum) {
  return new BoolLessSumConstraintDynamic<VarArray, VarSum>(_varArray, (SysInt)_varArray.size() -
                                                                            (SysInt)(_varSum));
}

template <typename VarArray, typename VarSum>
AbstractConstraint* BoolGreaterEqualSumConDynamic(const VarArray& _varArray, VarSum _varSum) {
  return new BoolLessSumConstraintDynamic<VarArray, VarSum, 0>(_varArray, _varSum);
}

#include "dynamic_sum_sat.h"

template <typename T>
inline AbstractConstraint* BuildCT_WATCHED_GEQSUM(const vector<T>& t1, ConstraintBlob& b) {
  for(SysInt i = 0; i < (SysInt)t1.size(); ++i) {
    if(t1[i].initialMin() < 0 || t1[i].initialMax() > 1)
      FAIL_EXIT("watched geqsum only works on Boolean variables!");
  }

  DomainInt sum = b.constants[0][0];
  if(sum == 1) {
    {
      return BoolSATConDynamic(t1);
    }
  } else {
    return BoolGreaterEqualSumConDynamic(t1, sum);
  }
}

/* JSON
  { "type": "constraint",
    "name": "watchsumgeq",
    "internal_name": "CT_WATCHED_GEQSUM",
    "args": [ "read_list", "read_constant" ]
  }
*/

template <typename T>
inline AbstractConstraint* BuildCT_WATCHED_LEQSUM(const vector<T>& t1, ConstraintBlob& b) {
  for(SysInt i = 0; i < (SysInt)t1.size(); ++i) {
    if(t1[i].initialMin() < 0 || t1[i].initialMax() > 1)
      FAIL_EXIT("watched leqsum only works on Boolean variables!");
  }

  return BoolLessEqualSumConDynamic(t1, checked_cast<SysInt>(b.constants[0][0]));
}

/* JSON
  { "type": "constraint",
    "name": "watchsumleq",
    "internal_name": "CT_WATCHED_LEQSUM",
    "args": [ "read_list", "read_constant" ]
  }
*/

#endif
