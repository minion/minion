// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0



















#ifndef CONSTRAINT_OCCURRENCE_H
#define CONSTRAINT_OCCURRENCE_H

// Negated occurrence; used in reverseConstraint for OccurrenceEqualConstraint
template <typename VarArray, typename Val, typename ValCount>
struct NotOccurrenceEqualConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "NotOccurrenceEqual";
  }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrencesCount;
  ReversibleInt not_occurrencesCount;
  VarArray varArray;

  ValCount valCount;
  Val value;

  CONSTRAINT_ARG_LIST3(varArray, value, valCount);

  NotOccurrenceEqualConstraint(const VarArray& _varArray, const Val& _value,
                               const ValCount& _valCount)
      : occurrencesCount(),
        not_occurrencesCount(),
        varArray(_varArray),
        valCount(_valCount),
        value(_value),
        trigger1index(-1),
        trigger2index(-1) {
    CheckNotBound(varArray, "occurrence");
    CheckNotBoundSingle(valCount, "occurrence");
  }

  // Put two assignment triggers on the vector, and one on valCount.
  // When all vars in X are assigned, remove count(X=v) from valCount

  // When valCount is assigned and all but one of the vector are assigned,
  // consider the remaining one in the vector, and either fix it to v or
  // remove v from its domain to avoid the value of valCount.

  virtual SysInt dynamicTriggerCount() { // two moving assignment triggers.
    return 3;
  }

  void triggerSetup() {
    moveTriggerInt(valCount, 2, Assigned);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    ;
    D_ASSERT(vSize == (SysInt)varArray.size() + 1);
    DomainInt count = 0;
    for(SysInt i = 0; i < vSize - 1; ++i)
      count += (*(v + i) == value);
    return count != *(v + vSize - 1);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size() + 1);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(AnyVarRef(varArray[i]));
    vars.push_back(AnyVarRef(valCount));
    return vars;
  }

  void propagateValCount() {
    // valCount has been assigned.
    if(trigger1index == -1 || varArray[trigger1index].isAssigned()) {
      trigger1index = watch_unassigned_inVector(-1, trigger1index, 0);
      if(trigger1index == -1) {
        valcountAssigned();
        return;
      }
    }
    if(trigger2index == -1 || varArray[trigger2index].isAssigned()) {
      trigger2index = watch_unassigned_inVector(trigger1index, trigger2index, 0);
      if(trigger2index == -1) {
        valcountAssigned();
        return;
      }
    }
  }

  virtual void propagateDynInt(SysInt trig, DomainDelta) {
    if(trig == 2) {
      propagateValCount();
      return;
    }

    if(trig == 0 || trigger1index == -1) {
      if(valCount.isAssigned()) {
        // make sure both triggers are in place.
        trigger1index = watch_unassigned_inVector(-1, trigger1index, 0);
        if(trigger1index == -1) {
          valcountAssigned();
          return;
        }
        if(trigger2index == -1 || varArray[trigger2index].isAssigned()) {
          trigger2index = watch_unassigned_inVector(trigger1index, trigger2index, 1);
          if(trigger2index == -1) {
            valcountAssigned();
            return;
          }
        }
      } else {
        trigger1index = watch_unassigned_inVector(-1, trigger1index, 0);
        if(trigger1index == -1) {
          vectorAssigned();
          return;
        }
      }
      return;
    }
    D_ASSERT(trig == 1);
    if(!valCount.isAssigned()) { // don't need two triggers.
      releaseTriggerInt(1);
      trigger2index = -1;
      return;
    }

    if(varArray[trigger1index].isAssigned()) {
      // just wait for the other trigger, then both triggers will be
      // repositioned. lazy coding!
      return;
    }

    trigger2index = watch_unassigned_inVector(trigger1index, trigger2index, 1);
    if(trigger2index == -1) {
      valcountAssigned();
    }
  }

  // unfinished new stuff starts here.

  SysInt watch_unassigned_inVector(SysInt avoidindex, SysInt oldsupport, DomainInt dt) {
    // move dt to an index other than avoidindex, or return -1.
    SysInt newsupport = oldsupport + 1;
    for(; newsupport < (SysInt)varArray.size(); newsupport++) {
      if(newsupport != avoidindex) {
        if(!varArray[newsupport].isAssigned()) {
          moveTriggerInt(varArray[newsupport], dt, Assigned);
          return newsupport;
        }
      }
    }

    for(newsupport = 0; newsupport <= oldsupport; newsupport++) {
      if(newsupport != avoidindex) {
        if(!varArray[newsupport].isAssigned()) {
          moveTriggerInt(varArray[newsupport], dt, Assigned);
          return newsupport;
        }
      }
    }
    return -1;
  }

  SysInt trigger1index;
  SysInt trigger2index;

  void vectorAssigned() {
    // count occurrences of val
    SysInt occ = 0;
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      if(varArray[i].assignedValue() == value)
        occ++;
    }
    if(valCount.inDomain(occ)) {
      valCount.removeFromDomain(occ);
    }
  }

  void valcountAssigned() {
    // valcount, and all but one (or all) of the vector, are assigned.
    // count occurrences of val
    SysInt occ = 0;
    SysInt unassigned = -1;
    D_ASSERT(valCount.isAssigned());
    for(SysInt i = 0; i < (SysInt)varArray.size(); i++) {
      if(varArray[i].isAssigned()) {
        if(varArray[i].assignedValue() == value) {
          occ++;
        }
      } else {
        D_ASSERT(unassigned == -1);
        unassigned = i;
      }
    }

    // and the rest.
    if(unassigned == -1) {
      // just check, everything is assigned.
      if(occ == valCount.assignedValue()) {
        getState().setFailed();
      }
    } else {
      if(occ == valCount.assignedValue()) { // need another occurrence of the value
        varArray[unassigned].assign(value);
      } else if(occ + 1 == valCount.assignedValue()) { // not allowed to
                                                           // have another
                                                           // value.
        varArray[unassigned].removeFromDomain(value);
      }
    }
  }

  virtual void fullPropagate() {
    triggerSetup();

    trigger1index = watch_unassigned_inVector(-1, -1, 0);
    if(trigger1index == -1) {
      vectorAssigned();
      return;
    }

    if(valCount.isAssigned()) {
      // watch a second place in the vector.
      trigger2index = watch_unassigned_inVector(trigger1index, trigger1index, 1);
      if(trigger2index == -1) {
        valcountAssigned();
        return;
      }
    }
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, varArray.size() + 1);

    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(!varArray[i].isAssigned()) {
        assignment.push_back(make_pair(i, varArray[i].min()));
        assignment.push_back(make_pair(i, varArray[i].max()));
        return true;
      } else
        c.push_back(varArray[i].assignedValue());
    }

    if(!valCount.isAssigned()) {
      assignment.push_back(make_pair(varArray.size(), valCount.min()));
      assignment.push_back(make_pair(varArray.size(), valCount.max()));
      return true;
    } else
      c.push_back(valCount.assignedValue());

    if(checkAssignment(c.begin(), c.size())) { // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)varArray.size() + 1; ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverseConstraint();
};

template <typename VarArray, typename Val>
struct ConstantOccurrenceEqualConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "OccurrenceLeq/Geq";
  }

  virtual string fullOutputName() {
    if(valCount_min == 0) {
      return ConOutput::printCon("occurrenceleq", varArray, value, valCountMax);
    } else {
      D_ASSERT(valCountMax == (SysInt)varArray.size());
      return ConOutput::printCon("occurrencegeq", varArray, value, valCount_min);
    }
  }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrencesCount;
  ReversibleInt not_occurrencesCount;
  VarArray varArray;

  DomainInt valCount_min;
  DomainInt valCountMax;
  Val value;

  ConstantOccurrenceEqualConstraint(const VarArray& _varArray, const Val& _value,
                                    DomainInt _valCount_min, DomainInt _valCountMax)
      : occurrencesCount(),
        not_occurrencesCount(),
        varArray(_varArray),
        valCount_min(_valCount_min),
        valCountMax(_valCountMax),
        value(_value) {
    occurrencesCount = 0;
    not_occurrencesCount = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return varArray.size();
  }

  void triggerSetup() {
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      moveTriggerInt(varArray[i], i, Assigned);
  }

  void occurrence_limit_reached() {
    D_ASSERT(valCountMax <= occurrencesCount);
    DomainInt occs = 0;
    typename VarArray::iterator end_it(varArray.end());
    for(typename VarArray::iterator it = varArray.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == (DomainInt)value)
          ++occs;
      } else {
        it->removeFromDomain(value);
      }
    }
    if(valCountMax < occs)
      getState().setFailed();
  }

  void not_occurrence_limit_reached() {
    D_ASSERT(not_occurrencesCount >=
             checked_cast<SysInt>((SysInt)varArray.size() - valCount_min));
    SysInt occs = 0;
    typename VarArray::iterator end_it(varArray.end());
    for(typename VarArray::iterator it = varArray.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() != (DomainInt)value)
          ++occs;
      } else {
        it->assign(value);
      }
    }
    if(valCount_min > static_cast<SysInt>(varArray.size()) - occs)
      getState().setFailed();
  }

  virtual void propagateDynInt(SysInt in, DomainDelta) {
    const SysInt i = checked_cast<SysInt>(in);
    PROP_INFO_ADDONE(OccEqual);
    D_ASSERT(i >= 0);

    if(varArray[i].assignedValue() == (DomainInt)value) {
      ++occurrencesCount;
      if(valCountMax < occurrencesCount)
        getState().setFailed();
      if(occurrencesCount == valCountMax)
        occurrence_limit_reached();
    } else {
      ++not_occurrencesCount;
      if(valCount_min > static_cast<SysInt>(varArray.size()) - not_occurrencesCount)
        getState().setFailed();
      if(not_occurrencesCount == static_cast<SysInt>(varArray.size()) - valCount_min)
        not_occurrence_limit_reached();
    }
  }

  void setupCounters() {
    SysInt occs = 0;
    SysInt not_occs = 0;
    typename VarArray::iterator end_it(varArray.end());
    for(typename VarArray::iterator it = varArray.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == (DomainInt)value)
          ++occs;
        else
          ++not_occs;
      }
    }
    occurrencesCount = occs;
    not_occurrencesCount = not_occs;
  }

  virtual void fullPropagate() {
    triggerSetup();
    if(valCountMax < 0 || valCount_min > (SysInt)varArray.size())
      getState().setFailed();
    setupCounters();

    if(valCountMax < occurrencesCount)
      getState().setFailed();

    if(valCount_min > static_cast<SysInt>(varArray.size()) - not_occurrencesCount)
      getState().setFailed();

    if(occurrencesCount == valCountMax)
      occurrence_limit_reached();
    if(not_occurrencesCount == static_cast<SysInt>(varArray.size()) - valCount_min)
      not_occurrence_limit_reached();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    DomainInt count = 0;
    for(SysInt i = 0; i < vSize; ++i)
      count += (*(v + i) == (DomainInt)value);
    return (count >= valCount_min) && (count <= valCountMax);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size());
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(AnyVarRef(varArray[i]));
    return vars;
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, varArray.size());

    if(valCount_min == 0) {
      DomainInt needVars = (DomainInt)varArray.size() - valCountMax;
      if(needVars <= 0)
        return true;
      for(int i = 0; i < (SysInt)varArray.size(); ++i) {
        if(varArray[i].min() != value) {
          assignment.push_back(make_pair(i, varArray[i].min()));
          needVars--;
        } else if(varArray[i].max() != value) {
          assignment.push_back(make_pair(i, varArray[i].max()));
          needVars--;
        }
        if(needVars == 0) {
          return true;
        }
      }
      assignment.clear();
      return false;
    } else if(valCountMax == (SysInt)varArray.size()) {
      DomainInt needVars = valCount_min;
      if(needVars <= 0)
        return true;
      for(int i = 0; i < (SysInt)varArray.size(); ++i) {
        if(varArray[i].inDomain(value)) {
          assignment.push_back(make_pair(i, value));
          needVars--;
        }
        if(needVars == 0)
          return true;
      }
      assignment.clear();
      return false;
    }

    abort();
    return false;
  }

  AbstractConstraint* reverseConstraint() {
    // This constraint actually constrains the occurrences of value to an an
    // interval
    // [valCount_min, valCountMax]. But it's apparently only used for
    // less-than
    // and greater-than. So identify the less-than case and make a greater-than,
    // etc.
    if(valCount_min == 0) {
      return new ConstantOccurrenceEqualConstraint<VarArray, Val>(
          varArray, value, valCountMax + 1, varArray.size());
    }
    if(valCountMax == (SysInt)varArray.size()) {
      return new ConstantOccurrenceEqualConstraint<VarArray, Val>(varArray, value, 0,
                                                                  valCount_min - 1);
    }
    FAIL_EXIT("Unable to negate an occurrence-interval constraint, sorry.");
    return NULL;
  }
};

template <typename VarArray, typename Val, typename ValCount>
struct OccurrenceEqualConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "occurrence";
  }

  CONSTRAINT_ARG_LIST3(varArray, value, valCount);

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrencesCount;
  ReversibleInt not_occurrencesCount;
  VarArray varArray;

  ValCount valCount;
  Val value;

  OccurrenceEqualConstraint(const VarArray& _varArray, const Val& _value,
                            const ValCount& _valCount)
      : occurrencesCount(),
        not_occurrencesCount(),
        varArray(_varArray),
        valCount(_valCount),
        value(_value) {
    occurrencesCount = 0;
    not_occurrencesCount = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return varArray.size() + 2;
  }

  void occurrence_limit_reached() {
    D_ASSERT(valCount.max() <= occurrencesCount);
    SysInt occs = 0;
    typename VarArray::iterator end_it(varArray.end());
    for(typename VarArray::iterator it = varArray.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == value)
          ++occs;
      } else {
        it->removeFromDomain(value);
      }
    }
    valCount.setMin(occs);
  }

  void not_occurrence_limit_reached() {
    D_ASSERT(not_occurrencesCount >= static_cast<SysInt>(varArray.size()) - valCount.min());
    SysInt occs = 0;
    typename VarArray::iterator end_it(varArray.end());
    for(typename VarArray::iterator it = varArray.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() != value)
          ++occs;
      } else {
        it->assign(value);
      }
    }
    valCount.setMax(static_cast<SysInt>(varArray.size()) - occs);
  }

  virtual void propagateDynInt(SysInt i, DomainDelta) {
    PROP_INFO_ADDONE(OccEqual);
    if(i >= varArray.size()) { // valCount changed
      if(occurrencesCount == valCount.max())
        occurrence_limit_reached();
      if(not_occurrencesCount == static_cast<SysInt>(varArray.size()) - valCount.min())
        not_occurrence_limit_reached();
      return;
    }

    if(varArray[checked_cast<SysInt>(i)].assignedValue() == value) {
      ++occurrencesCount;
      valCount.setMin((DomainInt)occurrencesCount);
      if(occurrencesCount == valCount.max())
        occurrence_limit_reached();
    } else {
      ++not_occurrencesCount;
      valCount.setMax(static_cast<SysInt>(varArray.size()) - not_occurrencesCount);
      if(not_occurrencesCount == static_cast<SysInt>(varArray.size()) - valCount.min())
        not_occurrence_limit_reached();
    }
  }

  void setupCounters() {
    SysInt occs = 0;
    SysInt not_occs = 0;
    typename VarArray::iterator end_it(varArray.end());
    for(typename VarArray::iterator it = varArray.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == value)
          ++occs;
        else
          ++not_occs;
      }
    }
    occurrencesCount = occs;
    not_occurrencesCount = not_occs;
  }

  void triggerSetup() {
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      moveTriggerInt(varArray[i], i, Assigned);
    moveTriggerInt(valCount, varArray.size(), UpperBound);
    moveTriggerInt(valCount, varArray.size() + 1, LowerBound);
  }

  virtual void fullPropagate() {
    triggerSetup();

    valCount.setMin(0);
    valCount.setMax(varArray.size());
    setupCounters();
    valCount.setMin((DomainInt)occurrencesCount);
    valCount.setMax((DomainInt)varArray.size() - not_occurrencesCount);

    if(occurrencesCount == valCount.max())
      occurrence_limit_reached();
    if(not_occurrencesCount == static_cast<SysInt>(varArray.size()) - valCount.min())
      not_occurrence_limit_reached();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size() + 1);
    DomainInt count = 0;
    for(SysInt i = 0; i < vSize - 1; ++i)
      count += (*(v + i) == value);
    return count == *(v + vSize - 1);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size() + 1);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(AnyVarRef(varArray[i]));
    vars.push_back(AnyVarRef(valCount));
    return vars;
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, varArray.size() + 1);

    for(SysInt i = 0; i < (SysInt)varArray.size(); ++i) {
      if(!varArray[i].isAssigned()) {
        assignment.push_back(make_pair(i, varArray[i].min()));
        assignment.push_back(make_pair(i, varArray[i].max()));
        return true;
      } else
        c.push_back(varArray[i].assignedValue());
    }

    if(!valCount.isAssigned()) {
      assignment.push_back(make_pair(varArray.size(), valCount.min()));
      assignment.push_back(make_pair(varArray.size(), valCount.max()));
      return true;
    } else
      c.push_back(valCount.assignedValue());

    if(checkAssignment(c.begin(), c.size())) { // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)varArray.size() + 1; ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverseConstraint() {
    return new NotOccurrenceEqualConstraint<VarArray, Val, ValCount>(varArray, value, valCount);
  }
};

template <typename VarArray, typename Val, typename ValCount>
AbstractConstraint* NotOccurrenceEqualConstraint<VarArray, Val, ValCount>::reverseConstraint() {
  return new OccurrenceEqualConstraint<VarArray, Val, ValCount>(varArray, value, valCount);
}

template <typename VarArray, typename Val, typename ValCount>
AbstractConstraint* OccEqualCon(const VarArray& _varArray, const Val& _value,
                                const ValCount& _valCount) {
  return (new OccurrenceEqualConstraint<VarArray, Val, ValCount>(_varArray, _value, _valCount));
}

template <typename VarArray, typename Val>
AbstractConstraint* ConstantOccEqualCon(const VarArray& _varArray, const Val& _value,
                                        DomainInt _valCount_min, DomainInt _valCountMax) {
  return (new ConstantOccurrenceEqualConstraint<VarArray, Val>(_varArray, _value, _valCount_min,
                                                               _valCountMax));
}

template <typename T1>
AbstractConstraint* BuildCT_GEQ_OCCURRENCE(const T1& t1, ConstraintBlob& b) {
  const SysInt valToCount = checked_cast<SysInt>(b.constants[0][0]);
  DomainInt occs = b.constants[1][0];
  { return ConstantOccEqualCon(t1, valToCount, occs, t1.size()); }
}

/* JSON
{ "type": "constraint",
  "name": "occurrencegeq",
  "internal_name": "CT_GEQ_OCCURRENCE",
  "args": [ "read_list", "read_constant", "read_constant" ]
}
*/

template <typename T1>
AbstractConstraint* BuildCT_LEQ_OCCURRENCE(const T1& t1, ConstraintBlob& b) {
  const SysInt valToCount = checked_cast<SysInt>(b.constants[0][0]);
  DomainInt occs = b.constants[1][0];
  return ConstantOccEqualCon(t1, valToCount, 0, occs);
}

/* JSON
{ "type": "constraint",
  "name": "occurrenceleq",
  "internal_name": "CT_LEQ_OCCURRENCE",
  "args": [ "read_list", "read_constant", "read_constant" ]
}
*/

template <typename T1, typename T3>
AbstractConstraint* BuildCT_OCCURRENCE(const T1& t1, const T3& t3, ConstraintBlob& b) {
  const SysInt valToCount = checked_cast<SysInt>(b.constants[0][0]);
  return OccEqualCon(t1, valToCount, t3[0]);
}

/* JSON
{ "type": "constraint",
  "name": "occurrence",
  "internal_name": "CT_OCCURRENCE",
  "args": [ "read_list", "read_constant", "read_var" ]
}
*/

#endif
