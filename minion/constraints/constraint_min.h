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

/** @help constraints;max Description
The constraint

   max(vec, x)

ensures that x is equal to the maximum value of any variable in vec.
*/

/** @help constraints;max References
See

   help constraints min

for the opposite constraint.
*/

/** @help constraints;min Description
The constraint

   min(vec, x)

ensures that x is equal to the minimum value of any variable in vec.
*/

/** @help constraints;min References
See

   help constraints max

for the opposite constraint.
*/

#ifndef CONSTRAINT_MIN_H
#define CONSTRAINT_MIN_H

#include "constraint_checkassign.h"

template <typename VarArray, typename MinVarRef>
struct MinConstraint : public AbstractConstraint {
  virtual string constraint_name() { return "min"; }

  virtual string full_output_name() {
    // We assume constraint is propagated here, we will do a simple check
    // to see if it is true.
    if (min_var.isAssigned()) {
      bool found_assigned_min = false;
      bool found_lesser_value = false;
      for (size_t i = 0; i < var_array.size(); ++i) {
        if (var_array[i].isAssigned() &&
            min_var.getAssignedValue() == var_array[i].getAssignedValue())
          found_assigned_min = true;
        if (var_array[i].getMin() < min_var.getMin())
          found_lesser_value = true;
      }
      if (found_assigned_min && !found_lesser_value)
        return "true()";
    }

    return ConOutput::print_reversible_con("min", "max", var_array, min_var);
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  typedef typename VarArray::value_type ArrayVarRef;

  VarArray var_array;
  MinVarRef min_var;

  MinConstraint(const VarArray &_var_array, const MinVarRef &_min_var)
      : var_array(_var_array), min_var(_min_var) {}

  virtual SysInt dynamic_trigger_count() {
    return (var_array.size() + 1) * 2;
  }

  void setup_triggers() {
    SysInt v_size = var_array.size();
    for (SysInt i = 0; i < v_size; ++i) {
      moveTriggerInt(var_array[i], i, LowerBound);
      moveTriggerInt(var_array[i], i + v_size + 1, UpperBound);
    }
    moveTriggerInt(min_var, v_size, LowerBound);
    moveTriggerInt(min_var, v_size * 2 + 1, UpperBound);
  }

  virtual void propagateDynInt(SysInt prop_val, DomainDelta) {
    PROP_INFO_ADDONE(Min);
    SysInt v_size = var_array.size();
    if (prop_val <= v_size) { // Lower Bound Changed
      // Had to add 1 to fix "0th array" problem.
      if (prop_val == v_size) {
        DomainInt new_min = min_var.getMin();
        typename VarArray::iterator end = var_array.end();
        for (typename VarArray::iterator it = var_array.begin(); it < end; ++it)
          (*it).setMin(new_min);
      } else {
        typename VarArray::iterator it = var_array.begin();
        typename VarArray::iterator end = var_array.end();
        DomainInt min = it->getMin();
        ++it;
        for (; it < end; ++it) {
          DomainInt it_min = it->getMin();
          if (it_min < min)
            min = it_min;
        }
        min_var.setMin(min);
      }
    } else { // Upper Bound Changed
      prop_val -= (v_size + 1);
      if (prop_val == v_size) {
        typename VarArray::iterator it = var_array.begin();
        DomainInt minvar_max = min_var.getMax();
        while (it != var_array.end() && (*it).getMin() > minvar_max)
          ++it;
        if (it == var_array.end()) {
          getState().setFailed(true);
          return;
        }
        // Possibly this variable is the only one that can be the minimum
        typename VarArray::iterator it_copy(it);
        ++it;
        while (it != var_array.end() && (*it).getMin() > minvar_max)
          ++it;
        if (it != var_array.end()) { // No, another variable can be the minimum
          return;
        }
        it_copy->setMax(minvar_max);
      } else {
        min_var.setMax(var_array[checked_cast<SysInt>(prop_val)].getMax());
      }
    }
  }

  virtual void full_propagate() {
    setup_triggers();
    SysInt array_size = var_array.size();
    if (array_size == 0) {
      getState().setFailed(true);
    } else {
      for (SysInt i = 0; i < (array_size+1) * 2; ++i) {
        propagateDynInt(i, DomainDelta::empty());
      }
    }
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)var_array.size() + 1);
    if (v_size == 1)
      return false;

    DomainInt min_val = v[0];
    for (SysInt i = 1; i < v_size - 1; i++)
      min_val = min(min_val, v[i]);
    return min_val == *(v + v_size - 1);
  }

  // Bah: This could be much better!
  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    for (DomainInt i = min_var.getMin(); i <= min_var.getMax(); ++i) {
      if (min_var.inDomain(i)) {
        bool flag_domain = false;
        for (SysInt j = 0; j < (SysInt)var_array.size(); ++j) {
          if (var_array[j].inDomain(i)) {
            flag_domain = true;
            assignment.push_back(make_pair(j, i));
          } else {
            if (var_array[j].getMax() < i) {
              return false;
            }
            if (var_array[j].getInitialMin() < i)
              assignment.push_back(make_pair(j, var_array[j].getMax()));
          }
        }

        if (flag_domain) {
          assignment.push_back(make_pair(var_array.size(), i));
          return true;
        } else
          assignment.clear();
      }
    }
    return false;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint *reverse_constraint() { return forward_check_negation(this); }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for (UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(min_var));
    return vars;
  }
};

template <typename VarArray, typename VarRef>
AbstractConstraint *BuildCT_MIN(const VarArray &_var_array, const std::vector<VarRef> &_var_ref,
                                ConstraintBlob &) {
  return (new MinConstraint<VarArray, VarRef>(_var_array, _var_ref[0]));
}

/* JSON
{ "type": "constraint",
  "name": "min",
  "internal_name": "CT_MIN",
  "args": [ "read_list", "read_var" ]
}
*/

template <typename VarArray, typename VarRef>
AbstractConstraint *BuildCT_MAX(const VarArray &_var_array, const vector<VarRef> &_var_ref,
                                ConstraintBlob &) {
  return (new MinConstraint<typename NegType<VarArray>::type, typename NegType<VarRef>::type>(
      VarNegRef(_var_array), VarNegRef(_var_ref[0])));
}

/* JSON
{ "type": "constraint",
  "name": "max",
  "internal_name": "CT_MAX",
  "args": [ "read_list", "read_var" ]
}
*/
#endif
