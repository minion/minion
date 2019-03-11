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

  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;

  ValCount val_count;
  Val value;

  CONSTRAINT_ARG_LIST3(var_array, value, val_count);

  NotOccurrenceEqualConstraint(const VarArray& _var_array, const Val& _value,
                               const ValCount& _val_count)
      : occurrences_count(),
        not_occurrences_count(),
        var_array(_var_array),
        val_count(_val_count),
        value(_value),
        trigger1index(-1),
        trigger2index(-1) {
    CheckNotBound(var_array, "occurrence");
    CheckNotBoundSingle(val_count, "occurrence");
  }

  // Put two assignment triggers on the vector, and one on val_count.
  // When all vars in X are assigned, remove count(X=v) from val_count

  // When val_count is assigned and all but one of the vector are assigned,
  // consider the remaining one in the vector, and either fix it to v or
  // remove v from its domain to avoid the value of val_count.

  virtual SysInt dynamicTriggerCount() { // two moving assignment triggers.
    return 3;
  }

  void trigger_setup() {
    moveTriggerInt(val_count, 2, Assigned);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt v_size) {
    ;
    D_ASSERT(v_size == (SysInt)var_array.size() + 1);
    DomainInt count = 0;
    for(SysInt i = 0; i < v_size - 1; ++i)
      count += (*(v + i) == value);
    return count != *(v + v_size - 1);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(val_count));
    return vars;
  }

  void propagateValCount() {
    // val_count has been assigned.
    if(trigger1index == -1 || var_array[trigger1index].isAssigned()) {
      trigger1index = watch_unassigned_in_vector(-1, trigger1index, 0);
      if(trigger1index == -1) {
        valcount_assigned();
        return;
      }
    }
    if(trigger2index == -1 || var_array[trigger2index].isAssigned()) {
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
      if(val_count.isAssigned()) {
        // make sure both triggers are in place.
        trigger1index = watch_unassigned_in_vector(-1, trigger1index, 0);
        if(trigger1index == -1) {
          valcount_assigned();
          return;
        }
        if(trigger2index == -1 || var_array[trigger2index].isAssigned()) {
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
    if(!val_count.isAssigned()) { // don't need two triggers.
      releaseTriggerInt(1);
      trigger2index = -1;
      return;
    }

    if(var_array[trigger1index].isAssigned()) {
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
    for(; newsupport < (SysInt)var_array.size(); newsupport++) {
      if(newsupport != avoidindex) {
        if(!var_array[newsupport].isAssigned()) {
          moveTriggerInt(var_array[newsupport], dt, Assigned);
          return newsupport;
        }
      }
    }

    for(newsupport = 0; newsupport <= oldsupport; newsupport++) {
      if(newsupport != avoidindex) {
        if(!var_array[newsupport].isAssigned()) {
          moveTriggerInt(var_array[newsupport], dt, Assigned);
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
    for(SysInt i = 0; i < (SysInt)var_array.size(); i++) {
      if(var_array[i].assignedValue() == value)
        occ++;
    }
    if(val_count.inDomain(occ)) {
      val_count.removeFromDomain(occ);
    }
  }

  void valcount_assigned() {
    // valcount, and all but one (or all) of the vector, are assigned.
    // count occurrences of val
    SysInt occ = 0;
    SysInt unassigned = -1;
    D_ASSERT(val_count.isAssigned());
    for(SysInt i = 0; i < (SysInt)var_array.size(); i++) {
      if(var_array[i].isAssigned()) {
        if(var_array[i].assignedValue() == value) {
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
      if(occ == val_count.assignedValue()) {
        getState().setFailed(true);
      }
    } else {
      if(occ == val_count.assignedValue()) { // need another occurrence of the value
        var_array[unassigned].assign(value);
      } else if(occ + 1 == val_count.assignedValue()) { // not allowed to
                                                           // have another
                                                           // value.
        var_array[unassigned].removeFromDomain(value);
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

    if(val_count.isAssigned()) {
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
    MAKE_STACK_BOX(c, DomainInt, var_array.size() + 1);

    for(SysInt i = 0; i < (SysInt)var_array.size(); ++i) {
      if(!var_array[i].isAssigned()) {
        assignment.push_back(make_pair(i, var_array[i].min()));
        assignment.push_back(make_pair(i, var_array[i].max()));
        return true;
      } else
        c.push_back(var_array[i].assignedValue());
    }

    if(!val_count.isAssigned()) {
      assignment.push_back(make_pair(var_array.size(), val_count.min()));
      assignment.push_back(make_pair(var_array.size(), val_count.max()));
      return true;
    } else
      c.push_back(val_count.assignedValue());

    if(checkAssignment(c.begin(), c.size())) { // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)var_array.size() + 1; ++i)
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
    if(val_count_min == 0) {
      return ConOutput::print_con("occurrenceleq", var_array, value, val_count_max);
    } else {
      D_ASSERT(val_count_max == (SysInt)var_array.size());
      return ConOutput::print_con("occurrencegeq", var_array, value, val_count_min);
    }
  }

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;

  DomainInt val_count_min;
  DomainInt val_count_max;
  Val value;

  ConstantOccurrenceEqualConstraint(const VarArray& _var_array, const Val& _value,
                                    DomainInt _val_count_min, DomainInt _val_count_max)
      : occurrences_count(),
        not_occurrences_count(),
        var_array(_var_array),
        val_count_min(_val_count_min),
        val_count_max(_val_count_max),
        value(_value) {
    occurrences_count = 0;
    not_occurrences_count = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return var_array.size();
  }

  void trigger_setup() {
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      moveTriggerInt(var_array[i], i, Assigned);
  }

  void occurrence_limit_reached() {
    D_ASSERT(val_count_max <= occurrences_count);
    DomainInt occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == (DomainInt)value)
          ++occs;
      } else {
        it->removeFromDomain(value);
      }
    }
    if(val_count_max < occs)
      getState().setFailed(true);
  }

  void not_occurrence_limit_reached() {
    D_ASSERT(not_occurrences_count >=
             checked_cast<SysInt>((SysInt)var_array.size() - val_count_min));
    SysInt occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() != (DomainInt)value)
          ++occs;
      } else {
        it->assign(value);
      }
    }
    if(val_count_min > static_cast<SysInt>(var_array.size()) - occs)
      getState().setFailed(true);
  }

  virtual void propagateDynInt(SysInt in, DomainDelta) {
    const SysInt i = checked_cast<SysInt>(in);
    PROP_INFO_ADDONE(OccEqual);
    D_ASSERT(i >= 0);

    if(var_array[i].assignedValue() == (DomainInt)value) {
      ++occurrences_count;
      if(val_count_max < occurrences_count)
        getState().setFailed(true);
      if(occurrences_count == val_count_max)
        occurrence_limit_reached();
    } else {
      ++not_occurrences_count;
      if(val_count_min > static_cast<SysInt>(var_array.size()) - not_occurrences_count)
        getState().setFailed(true);
      if(not_occurrences_count == static_cast<SysInt>(var_array.size()) - val_count_min)
        not_occurrence_limit_reached();
    }
  }

  void setup_counters() {
    SysInt occs = 0;
    SysInt not_occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == (DomainInt)value)
          ++occs;
        else
          ++not_occs;
      }
    }
    occurrences_count = occs;
    not_occurrences_count = not_occs;
  }

  virtual void fullPropagate() {
    trigger_setup();
    if(val_count_max < 0 || val_count_min > (SysInt)var_array.size())
      getState().setFailed(true);
    setup_counters();

    if(val_count_max < occurrences_count)
      getState().setFailed(true);

    if(val_count_min > static_cast<SysInt>(var_array.size()) - not_occurrences_count)
      getState().setFailed(true);

    if(occurrences_count == val_count_max)
      occurrence_limit_reached();
    if(not_occurrences_count == static_cast<SysInt>(var_array.size()) - val_count_min)
      not_occurrence_limit_reached();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)var_array.size());
    DomainInt count = 0;
    for(SysInt i = 0; i < v_size; ++i)
      count += (*(v + i) == (DomainInt)value);
    return (count >= val_count_min) && (count <= val_count_max);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, var_array.size());

    if(val_count_min == 0) {
      DomainInt need_vars = (DomainInt)var_array.size() - val_count_max;
      if(need_vars <= 0)
        return true;
      for(int i = 0; i < (SysInt)var_array.size(); ++i) {
        if(var_array[i].min() != value) {
          assignment.push_back(make_pair(i, var_array[i].min()));
          need_vars--;
        } else if(var_array[i].max() != value) {
          assignment.push_back(make_pair(i, var_array[i].max()));
          need_vars--;
        }
        if(need_vars == 0) {
          return true;
        }
      }
      assignment.clear();
      return false;
    } else if(val_count_max == (SysInt)var_array.size()) {
      DomainInt need_vars = val_count_min;
      if(need_vars <= 0)
        return true;
      for(int i = 0; i < (SysInt)var_array.size(); ++i) {
        if(var_array[i].inDomain(value)) {
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
    // [val_count_min, val_count_max]. But it's apparently only used for
    // less-than
    // and greater-than. So identify the less-than case and make a greater-than,
    // etc.
    if(val_count_min == 0) {
      return new ConstantOccurrenceEqualConstraint<VarArray, Val>(
          var_array, value, val_count_max + 1, var_array.size());
    }
    if(val_count_max == (SysInt)var_array.size()) {
      return new ConstantOccurrenceEqualConstraint<VarArray, Val>(var_array, value, 0,
                                                                  val_count_min - 1);
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

  CONSTRAINT_ARG_LIST3(var_array, value, val_count);

  typedef typename VarArray::value_type VarRef;

  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;

  ValCount val_count;
  Val value;

  OccurrenceEqualConstraint(const VarArray& _var_array, const Val& _value,
                            const ValCount& _val_count)
      : occurrences_count(),
        not_occurrences_count(),
        var_array(_var_array),
        val_count(_val_count),
        value(_value) {
    occurrences_count = 0;
    not_occurrences_count = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return var_array.size() + 2;
  }

  void occurrence_limit_reached() {
    D_ASSERT(val_count.max() <= occurrences_count);
    SysInt occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == value)
          ++occs;
      } else {
        it->removeFromDomain(value);
      }
    }
    val_count.setMin(occs);
  }

  void not_occurrence_limit_reached() {
    D_ASSERT(not_occurrences_count >= static_cast<SysInt>(var_array.size()) - val_count.min());
    SysInt occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() != value)
          ++occs;
      } else {
        it->assign(value);
      }
    }
    val_count.setMax(static_cast<SysInt>(var_array.size()) - occs);
  }

  virtual void propagateDynInt(SysInt i, DomainDelta) {
    PROP_INFO_ADDONE(OccEqual);
    if(i >= var_array.size()) { // val_count changed
      if(occurrences_count == val_count.max())
        occurrence_limit_reached();
      if(not_occurrences_count == static_cast<SysInt>(var_array.size()) - val_count.min())
        not_occurrence_limit_reached();
      return;
    }

    if(var_array[checked_cast<SysInt>(i)].assignedValue() == value) {
      ++occurrences_count;
      val_count.setMin((DomainInt)occurrences_count);
      if(occurrences_count == val_count.max())
        occurrence_limit_reached();
    } else {
      ++not_occurrences_count;
      val_count.setMax(static_cast<SysInt>(var_array.size()) - not_occurrences_count);
      if(not_occurrences_count == static_cast<SysInt>(var_array.size()) - val_count.min())
        not_occurrence_limit_reached();
    }
  }

  void setup_counters() {
    SysInt occs = 0;
    SysInt not_occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it) {
      if(it->isAssigned()) {
        if(it->assignedValue() == value)
          ++occs;
        else
          ++not_occs;
      }
    }
    occurrences_count = occs;
    not_occurrences_count = not_occs;
  }

  void trigger_setup() {
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      moveTriggerInt(var_array[i], i, Assigned);
    moveTriggerInt(val_count, var_array.size(), UpperBound);
    moveTriggerInt(val_count, var_array.size() + 1, LowerBound);
  }

  virtual void fullPropagate() {
    trigger_setup();

    val_count.setMin(0);
    val_count.setMax(var_array.size());
    setup_counters();
    val_count.setMin((DomainInt)occurrences_count);
    val_count.setMax((DomainInt)var_array.size() - not_occurrences_count);

    if(occurrences_count == val_count.max())
      occurrence_limit_reached();
    if(not_occurrences_count == static_cast<SysInt>(var_array.size()) - val_count.min())
      not_occurrence_limit_reached();
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)var_array.size() + 1);
    DomainInt count = 0;
    for(SysInt i = 0; i < v_size - 1; ++i)
      count += (*(v + i) == value);
    return count == *(v + v_size - 1);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(val_count));
    return vars;
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, var_array.size() + 1);

    for(SysInt i = 0; i < (SysInt)var_array.size(); ++i) {
      if(!var_array[i].isAssigned()) {
        assignment.push_back(make_pair(i, var_array[i].min()));
        assignment.push_back(make_pair(i, var_array[i].max()));
        return true;
      } else
        c.push_back(var_array[i].assignedValue());
    }

    if(!val_count.isAssigned()) {
      assignment.push_back(make_pair(var_array.size(), val_count.min()));
      assignment.push_back(make_pair(var_array.size(), val_count.max()));
      return true;
    } else
      c.push_back(val_count.assignedValue());

    if(checkAssignment(c.begin(), c.size())) { // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)var_array.size() + 1; ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }

  AbstractConstraint* reverseConstraint() {
    return new NotOccurrenceEqualConstraint<VarArray, Val, ValCount>(var_array, value, val_count);
  }
};

template <typename VarArray, typename Val, typename ValCount>
AbstractConstraint* NotOccurrenceEqualConstraint<VarArray, Val, ValCount>::reverseConstraint() {
  return new OccurrenceEqualConstraint<VarArray, Val, ValCount>(var_array, value, val_count);
}

template <typename VarArray, typename Val, typename ValCount>
AbstractConstraint* OccEqualCon(const VarArray& _var_array, const Val& _value,
                                const ValCount& _val_count) {
  return (new OccurrenceEqualConstraint<VarArray, Val, ValCount>(_var_array, _value, _val_count));
}

template <typename VarArray, typename Val>
AbstractConstraint* ConstantOccEqualCon(const VarArray& _var_array, const Val& _value,
                                        DomainInt _val_count_min, DomainInt _val_count_max) {
  return (new ConstantOccurrenceEqualConstraint<VarArray, Val>(_var_array, _value, _val_count_min,
                                                               _val_count_max));
}

template <typename T1>
AbstractConstraint* BuildCT_GEQ_OCCURRENCE(const T1& t1, ConstraintBlob& b) {
  const SysInt val_to_count = checked_cast<SysInt>(b.constants[0][0]);
  DomainInt occs = b.constants[1][0];
  { return ConstantOccEqualCon(t1, val_to_count, occs, t1.size()); }
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
  const SysInt val_to_count = checked_cast<SysInt>(b.constants[0][0]);
  DomainInt occs = b.constants[1][0];
  return ConstantOccEqualCon(t1, val_to_count, 0, occs);
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
  const SysInt val_to_count = checked_cast<SysInt>(b.constants[0][0]);
  return OccEqualCon(t1, val_to_count, t3[0]);
}

/* JSON
{ "type": "constraint",
  "name": "occurrence",
  "internal_name": "CT_OCCURRENCE",
  "args": [ "read_list", "read_constant", "read_var" ]
}
*/

#endif
