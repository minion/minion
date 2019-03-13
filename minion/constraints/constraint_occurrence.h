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

/** @help constraints;occurrence Description
The constraint

   occurrence(vec, elem, count)

ensures that there are count occurrences of the value elem in the
vector vec.
*/

/** @help constraints;occurrence Notes
elem must be a constant, not a variable.
*/

/** @help constraints;occurrence References
help constraints occurrenceleq
help constraints occurrencegeq
*/

/** @help constraints;occurrenceleq Description
The constraint

   occurrenceleq(vec, elem, count)

ensures that there are AT MOST count occurrences of the value elem in
the vector vec.
*/

/** @help constraints;occurrenceleq Notes
elem and count must be constants
*/

/** @help constraints;occurrenceleq References
help constraints occurrence
help constraints occurrencegeq
*/

/** @help constraints;occurrencegeq Description
The constraint

   occurrencegeq(vec, elem, count)

ensures that there are AT LEAST count occurrences of the value elem in
the vector vec.
*/

/** @help constraints;occurrencegeq Notes
elem and count must be constants
*/

/** @help constraints;occurrencegeq References
help constraints occurrence
help constraints occurrenceleq
*/

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

  void trigger_setup() {
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
      trigger1index = watch_unassigned_in_vector(-1, trigger1index, 0);
      if(trigger1index == -1) {
        valcount_assigned();
        return;
      }
    }
    if(trigger2index == -1 || varArray[trigger2index].isAssigned()) {
      trigger2index = watch_unassigned_in_vector(trigger1index, trigger2index, 0);
      if(trigger2index == -1) {
        valcount_assigned();
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
        trigger1index = watch_unassigned_in_vector(-1, trigger1index, 0);
        if(trigger1index == -1) {
          valcount_assigned();
          return;
        }
        if(trigger2index == -1 || varArray[trigger2index].isAssigned()) {
          trigger2index = watch_unassigned_in_vector(trigger1index, trigger2index, 1);
          if(trigger2index == -1) {
            valcount_assigned();
            return;
          }
        }
      } else {
        trigger1index = watch_unassigned_in_vector(-1, trigger1index, 0);
        if(trigger1index == -1) {
          vector_assigned();
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

    trigger2index = watch_unassigned_in_vector(trigger1index, trigger2index, 1);
    if(trigger2index == -1) {
      valcount_assigned();
    }
  }

  // unfinished new stuff starts here.

  SysInt watch_unassigned_in_vector(SysInt avoidindex, SysInt oldsupport, DomainInt dt) {
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

  void vector_assigned() {
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

  void valcount_assigned() {
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
        getState().setFailed(true);
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
    trigger_setup();

    trigger1index = watch_unassigned_in_vector(-1, -1, 0);
    if(trigger1index == -1) {
      vector_assigned();
      return;
    }

    if(valCount.isAssigned()) {
      // watch a second place in the vector.
      trigger2index = watch_unassigned_in_vector(trigger1index, trigger1index, 1);
      if(trigger2index == -1) {
        valcount_assigned();
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
      return ConOutput::print_con("occurrenceleq", varArray, value, valCount_max);
    } else {
      D_ASSERT(valCount_max == (SysInt)varArray.size());
      return ConOutput::print_con("occurrencegeq", varArray, value, valCount_min);
    }
  }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrencesCount;
  ReversibleInt not_occurrencesCount;
  VarArray varArray;

  DomainInt valCount_min;
  DomainInt valCount_max;
  Val value;

  ConstantOccurrenceEqualConstraint(const VarArray& _varArray, const Val& _value,
                                    DomainInt _valCount_min, DomainInt _valCount_max)
      : occurrencesCount(),
        not_occurrencesCount(),
        varArray(_varArray),
        valCount_min(_valCount_min),
        valCount_max(_valCount_max),
        value(_value) {
    occurrencesCount = 0;
    not_occurrencesCount = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return varArray.size();
  }

  void trigger_setup() {
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      moveTriggerInt(varArray[i], i, Assigned);
  }

  void occurrence_limit_reached() {
    D_ASSERT(valCount_max <= occurrencesCount);
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
    if(valCount_max < occs)
      getState().setFailed(true);
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
      getState().setFailed(true);
  }

  virtual void propagateDynInt(SysInt in, DomainDelta) {
    const SysInt i = checked_cast<SysInt>(in);
    PROP_INFO_ADDONE(OccEqual);
    D_ASSERT(i >= 0);

    if(varArray[i].assignedValue() == (DomainInt)value) {
      ++occurrencesCount;
      if(valCount_max < occurrencesCount)
        getState().setFailed(true);
      if(occurrencesCount == valCount_max)
        occurrence_limit_reached();
    } else {
      ++not_occurrencesCount;
      if(valCount_min > static_cast<SysInt>(varArray.size()) - not_occurrencesCount)
        getState().setFailed(true);
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
    trigger_setup();
    if(valCount_max < 0 || valCount_min > (SysInt)varArray.size())
      getState().setFailed(true);
    setupCounters();

    if(valCount_max < occurrencesCount)
      getState().setFailed(true);

    if(valCount_min > static_cast<SysInt>(varArray.size()) - not_occurrencesCount)
      getState().setFailed(true);

    if(occurrencesCount == valCount_max)
      occurrence_limit_reached();
    if(not_occurrencesCount == static_cast<SysInt>(varArray.size()) - valCount_min)
      not_occurrence_limit_reached();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size());
    DomainInt count = 0;
    for(SysInt i = 0; i < vSize; ++i)
      count += (*(v + i) == (DomainInt)value);
    return (count >= valCount_min) && (count <= valCount_max);
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
      DomainInt need_vars = (DomainInt)varArray.size() - valCount_max;
      if(need_vars <= 0)
        return true;
      for(int i = 0; i < (SysInt)varArray.size(); ++i) {
        if(varArray[i].min() != value) {
          assignment.push_back(make_pair(i, varArray[i].min()));
          need_vars--;
        } else if(varArray[i].max() != value) {
          assignment.push_back(make_pair(i, varArray[i].max()));
          need_vars--;
        }
        if(need_vars == 0) {
          return true;
        }
      }
      assignment.clear();
      return false;
    } else if(valCount_max == (SysInt)varArray.size()) {
      DomainInt need_vars = valCount_min;
      if(need_vars <= 0)
        return true;
      for(int i = 0; i < (SysInt)varArray.size(); ++i) {
        if(varArray[i].inDomain(value)) {
          assignment.push_back(make_pair(i, value));
          need_vars--;
        }
        if(need_vars == 0)
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
    // [valCount_min, valCount_max]. But it's apparently only used for
    // less-than
    // and greater-than. So identify the less-than case and make a greater-than,
    // etc.
    if(valCount_min == 0) {
      return new ConstantOccurrenceEqualConstraint<VarArray, Val>(
          varArray, value, valCount_max + 1, varArray.size());
    }
    if(valCount_max == (SysInt)varArray.size()) {
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

  void trigger_setup() {
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      moveTriggerInt(varArray[i], i, Assigned);
    moveTriggerInt(valCount, varArray.size(), UpperBound);
    moveTriggerInt(valCount, varArray.size() + 1, LowerBound);
  }

  virtual void fullPropagate() {
    trigger_setup();

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
                                        DomainInt _valCount_min, DomainInt _valCount_max) {
  return (new ConstantOccurrenceEqualConstraint<VarArray, Val>(_varArray, _value, _valCount_min,
                                                               _valCount_max));
}

template <typename T1>
AbstractConstraint* BuildCT_GEQ_OCCURRENCE(const T1& t1, ConstraintBlob& b) {
  const SysInt val_toCount = checked_cast<SysInt>(b.constants[0][0]);
  DomainInt occs = b.constants[1][0];
  { return ConstantOccEqualCon(t1, val_toCount, occs, t1.size()); }
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
  const SysInt val_toCount = checked_cast<SysInt>(b.constants[0][0]);
  DomainInt occs = b.constants[1][0];
  return ConstantOccEqualCon(t1, val_toCount, 0, occs);
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
  const SysInt val_toCount = checked_cast<SysInt>(b.constants[0][0]);
  return OccEqualCon(t1, val_toCount, t3[0]);
}

/* JSON
{ "type": "constraint",
  "name": "occurrence",
  "internal_name": "CT_OCCURRENCE",
  "args": [ "read_list", "read_constant", "read_var" ]
}
*/

#endif
