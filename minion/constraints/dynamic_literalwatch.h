// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0









#ifndef CONSTRAINT_DYNAMIC_LITWATCH_H
#define CONSTRAINT_DYNAMIC_LITWATCH_H

#include "constraint_checkassign.h"

template <typename VarArray, typename ValueArray, typename VarSum>
struct LiteralSumConstraintDynamic : public AbstractConstraint {
  virtual string constraintName() {
    return "litsumgeq";
  }

  typedef typename VarArray::value_type VarRef;

  VarArray varArray;
  ValueArray value_array;
  void* unwatchedIndexes;
  DomainInt last;
  DomainInt numUnwatched;

  CONSTRAINT_ARG_LIST3(varArray, value_array, varSum);

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }

  SysInt& unwatched(DomainInt i) {
    return static_cast<SysInt*>(unwatchedIndexes)[checked_cast<SysInt>(i)];
  }

  VarSum varSum;

  LiteralSumConstraintDynamic(const VarArray& _varArray, ValueArray _val_array, VarSum _varSum)
      : varArray(_varArray), value_array(_val_array), varSum(_varSum) {
    SysInt arraySize = varArray.size();

    numUnwatched = arraySize - varSum - 1;
    if(numUnwatched < 0)
      numUnwatched = 0;
    if(numUnwatched > (SysInt)varArray.size())
      numUnwatched = varArray.size();

    unwatchedIndexes =
        checked_malloc(checked_cast<SysInt>(sizeof(UnsignedSysInt) * numUnwatched));
    // above line might request 0 bytes
    last = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    if(varSum <= 0)
      return 0;
    if(varSum > (SysInt)varArray.size())
      return varArray.size() + 1;
    return checked_cast<SysInt>(varSum + 1);
  }

  virtual void fullPropagate() {
    if(varSum <= 0)
      return;

    DomainInt trigPos = 0;

    SysInt arraySize = varArray.size();
    DomainInt triggers_wanted = varSum + 1;
    SysInt index;

    for(index = 0; (index < arraySize) && (triggers_wanted > 0); ++index) {
      if(varArray[index].inDomain(value_array[index])) {
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
        if(varArray[i].inDomain(value_array[i])) {
          varArray[i].assign(value_array[i]);
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
        if(varArray[i].inDomain(value_array[i])) {
          triggerInfo(trigPos) = i;
          moveTriggerInt(varArray[i], trigPos, DomainRemoval, value_array[i]);
          ++trigPos;
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
    PROP_INFO_ADDONE(DynLitWatch);
    D_ASSERT(check_consistency());
    SysInt propval = triggerInfo(dt);

    D_ASSERT(!varArray[propval].inDomain(value_array[propval]));

    BOOL foundNewSupport = false;

    DomainInt j = 0;

    for(SysInt loop = 0; (!foundNewSupport) && loop < numUnwatched;) {
      D_ASSERT(numUnwatched > 0);

      j = (last + 1 + loop) % numUnwatched;
      if(varArray[unwatched(j)].inDomain(value_array[unwatched(j)])) {
        foundNewSupport = true;
      }
      // XXX What is going on here? check in dynamic_sum!
      { ++loop; }
    }

    if(foundNewSupport) // so we have found a new literal to watch
    {
      SysInt& unwatchedIndex = unwatched(j);

      // propval gives array index of old watched lit

      triggerInfo(dt) = unwatchedIndex;
      moveTriggerInt(varArray[unwatchedIndex], dt, DomainRemoval, value_array[unwatchedIndex]);

      unwatchedIndex = propval;
      last = j;
      return;
    }

    // there is no literal to watch, we need to propagate

    SysInt dt2 = 0;

    for(SysInt z = 0; z < varSum + 1; ++z) {
      if(dt != dt2) // that one has just been set the other way
      {
        varArray[triggerInfo(dt2)].assign(value_array[triggerInfo(dt2)]);
      }
      dt2++;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    SysInt count = 0;
    for(SysInt i = 0; i < vSize; ++i)
      count += (v[i] == value_array[i]);
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
    if(varSum <= 0)
      return true;
    SysInt lit_matchCount = 0;

    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(varArray[i].inDomain(value_array[i])) {
        assignment.push_back(make_pair(i, value_array[i]));
        lit_matchCount++;
        if(lit_matchCount == varSum)
          return true;
      }
    }
    return false;
  }
};

template <typename VarArray, typename ValArray, typename VarSum>
AbstractConstraint* LiteralSumConDynamic(const VarArray& _varArray, const ValArray& _val_array,
                                         VarSum _varSum) {
  return new LiteralSumConstraintDynamic<VarArray, ValArray, VarSum>(_varArray, _val_array,
                                                                     _varSum);
}

template <typename T1>
AbstractConstraint* BuildCT_WATCHED_LITSUM(const T1& t1, ConstraintBlob& b) {
  D_ASSERT(b.constants[1].size());
  return LiteralSumConDynamic(t1, b.constants[0], checked_cast<SysInt>(b.constants[1][0]));
}

/* JSON
  { "type": "constraint",
    "name": "litsumgeq",
    "internal_name": "CT_WATCHED_LITSUM",
    "args": [ "read_list", "read_constant_list", "read_constant" ]
  }
*/

#endif
