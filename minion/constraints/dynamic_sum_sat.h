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

#ifndef CONSTRAINT_DYNAMIC_SUM_SAT_H
#define CONSTRAINT_DYNAMIC_SUM_SAT_H

template <typename VarArray>
struct BoolSATConstraintDynamic : public AbstractConstraint {
  virtual string constraintName() {
    return "watchsumgeq";
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new BoolLessSumConstraintDynamic<VarArray, DomainInt, 1>(var_array, var_array.size());
  }

  typedef typename VarArray::value_type VarRef;

  CONSTRAINT_ARG_LIST2(var_array, (DomainInt)1);

  VarArray var_array;

  SysInt last;

  BoolSATConstraintDynamic(const VarArray& _var_array) : var_array(_var_array) {
    last = 0;
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    SysInt array_size = var_array.size();
    SysInt trig1, trig2;
    SysInt index = 0;

    while(index < array_size && !var_array[index].inDomain(1))
      ++index;

    trig1 = index;

    if(index == array_size) { // Not enough triggers
      getState().setFailed(true);
      return;
    }

    ++index;

    while(index < array_size && !var_array[index].inDomain(1))
      ++index;

    trig2 = index;

    if(index >= array_size) { // Only one valid variable.
      var_array[trig1].assign(1);
      return;
    }

    triggerInfo(0) = trig1;
    moveTriggerInt(var_array[trig1], 0, UpperBound);

    triggerInfo(1) = trig2;
    moveTriggerInt(var_array[trig2], 1, UpperBound);

    return;
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(DynSumSat);
    SysInt var_size = var_array.size();

    SysInt other_propval;

    if(0 == dt)
      other_propval = triggerInfo(1);
    else
      other_propval = triggerInfo(0);

    // I thought this would make the code go faster. But it doesn't!
    //  if(var_array[other_propval].isAssignedValue(1))
    //    return;

    bool found_new_support = false;

    SysInt loop = last;

    while(loop < var_size && !found_new_support) {
      if(loop != other_propval && var_array[loop].inDomain(1))
        found_new_support = true;
      else
        ++loop;
    }

    if(!found_new_support) {
      loop = 0;

      while(loop < last && !found_new_support) {
        if(loop != other_propval && var_array[loop].inDomain(1))
          found_new_support = true;
        else
          ++loop;
      }

      if(!found_new_support) { // Have to propagate!
        var_array[other_propval].assign(1);
        return;
      }
    }

    // Found new value to watch
    triggerInfo(dt) = loop;
    last = loop;
    moveTriggerInt(var_array[loop], dt, UpperBound);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)var_array.size());
    SysInt count = 0;
    for(SysInt i = 0; i < v_size; ++i)
      count += (v[i] == 1);
    return count > 0;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(SysInt i = 0; i < (SysInt)var_array.size(); ++i) {
      if(var_array[i].inDomain(1)) {
        assignment.push_back(make_pair(i, 1));
        return true;
      }
    }
    return false;
  }
};

template <typename VarArray>
AbstractConstraint* BoolSATConDynamic(const VarArray& _var_array) {
  return new BoolSATConstraintDynamic<VarArray>(_var_array);
}

#endif
